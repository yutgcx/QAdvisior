#include "qir_generator.hpp"
#include <iostream>
#include <fstream>

// Include LLVM passes for optimization
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

namespace qadvisor {

QIRGenerator::QIRGenerator(const std::string& module_name, int opt_level)
    : context_(std::make_unique<llvm::LLVMContext>())
    , builder_(*context_)
    , opt_level_(opt_level)
    , hFunc_(nullptr)
    , xFunc_(nullptr)
    , zFunc_(nullptr)
    , cnotFunc_(nullptr)
    , measureFunc_(nullptr)
    , readResultFunc_(nullptr)
{
    module_ = new llvm::Module(module_name, *context_);
    type_manager_ = std::make_unique<TypeManager>(*context_);
    metadata_builder_ = std::make_unique<MetadataBuilder>(*context_);
}

QIRGenerator::~QIRGenerator() {
    delete module_;
}

bool QIRGenerator::generate(ProgramAST* ast, Schedule* schedule, DependencyGraph* graph) {
    if (!ast || !schedule || !graph) {
        llvm::errs() << "Error: Null AST, schedule, or graph\n";
        return false;
    }
    
    // Count qubits and classical bits from AST
    num_qubits_ = 0;
    num_classical_bits_ = 0;
    
    QregDecl* q = ast->qubit_registers;
    while (q) {
        num_qubits_ += q->size;
        q = q->next;
    }
    
    CregDecl* c = ast->classical_registers;
    while (c) {
        num_classical_bits_ += c->size;
        c = c->next;
    }
    
    // Initialize managers
    qubit_manager_ = std::make_unique<QubitManager>(num_qubits_, *context_, builder_);
    
    // Declare all quantum functions
    declareQuantumFunctions();
    declareRuntimeFunctions();
    
    // Create main function
    createMainFunction();
    
    // Emit qubit allocations
    qubit_manager_->allocateAll();
    
    // Emit gates according to schedule (parallel groups)
    for (int g = 0; g < schedule->num_groups; g++) {
        emitParallelGroup(schedule->groups[g], graph);
    }
    
    // Return from main
    builder_.CreateRetVoid();
    
    // Verify IR
    if (!verify()) {
        return false;
    }
    
    // Add optimization passes if requested
    if (opt_level_ > 0) {
        addOptimizationPasses();
    }
    
    return true;
}

void QIRGenerator::declareQuantumFunctions() {
    // Get types from type manager
    llvm::Type* qubitPtrType = type_manager_->getQubitPtrType();
    llvm::Type* resultPtrType = type_manager_->getResultPtrType();
    llvm::Type* voidType = type_manager_->getVoidType();
    
    // Hadamard: void @__quantum__qis__h__body(%Qubit*)
    auto hType = llvm::FunctionType::get(voidType, {qubitPtrType}, false);
    hFunc_ = llvm::Function::Create(hType, 
        llvm::Function::ExternalLinkage,
        "__quantum__qis__h__body", module_);
    
    // Pauli X: void @__quantum__qis__x__body(%Qubit*)
    auto xType = llvm::FunctionType::get(voidType, {qubitPtrType}, false);
    xFunc_ = llvm::Function::Create(xType,
        llvm::Function::ExternalLinkage,
        "__quantum__qis__x__body", module_);
    
    // Pauli Z: void @__quantum__qis__z__body(%Qubit*)
    auto zType = llvm::FunctionType::get(voidType, {qubitPtrType}, false);
    zFunc_ = llvm::Function::Create(zType,
        llvm::Function::ExternalLinkage,
        "__quantum__qis__z__body", module_);
    
    // CNOT: void @__quantum__qis__cnot__body(%Qubit*, %Qubit*)
    auto cnotType = llvm::FunctionType::get(voidType, 
        {qubitPtrType, qubitPtrType}, false);
    cnotFunc_ = llvm::Function::Create(cnotType,
        llvm::Function::ExternalLinkage,
        "__quantum__qis__cnot__body", module_);
    
    // Measure: void @__quantum__qis__mz__body(%Qubit*, %Result*)
    auto measureType = llvm::FunctionType::get(voidType,
        {qubitPtrType, resultPtrType}, false);
    measureFunc_ = llvm::Function::Create(measureType,
        llvm::Function::ExternalLinkage,
        "__quantum__qis__mz__body", module_);
    
    // Read result: i1 @__quantum__qis__read_result__body(%Result*)
    auto readType = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(*context_),
        {resultPtrType}, false);
    readResultFunc_ = llvm::Function::Create(readType,
        llvm::Function::ExternalLinkage,
        "__quantum__qis__read_result__body", module_);
}

void QIRGenerator::declareRuntimeFunctions() {
    // For array allocation if needed
    // (Simplified - actual QIR has more runtime functions)
}

void QIRGenerator::createMainFunction() {
    // main() returns void in QIR
    auto funcType = llvm::FunctionType::get(
        type_manager_->getVoidType(), false);
    
    auto mainFunc = llvm::Function::Create(funcType,
        llvm::Function::ExternalLinkage,
        "main", module_);
    
    // Create entry block
    auto entryBB = llvm::BasicBlock::Create(*context_, "entry", mainFunc);
    builder_.SetInsertPoint(entryBB);
}

void QIRGenerator::createEntryBlock() {
    // Already set in createMainFunction
}

void QIRGenerator::createReturnBlock() {
    // Void return is already handled
}

llvm::Value* QIRGenerator::getQubitPtr(int qubit_index) {
    return qubit_manager_->getQubitPtr(qubit_index);
}

llvm::Value* QIRGenerator::getResultPtr(int classical_bit) {
    auto it = classical_results_.find(classical_bit);
    if (it != classical_results_.end()) {
        return it->second;
    }
    
    // QIR Result types are generated as cast pointers
    llvm::Type* int64_type = type_manager_->getInt64Type();
    llvm::Value* int_val = llvm::ConstantInt::get(int64_type, classical_bit);
    llvm::Value* resultPtr = builder_.CreateIntToPtr(int_val, type_manager_->getResultPtrType(), "result_" + std::to_string(classical_bit));
    
    classical_results_[classical_bit] = resultPtr;
    return resultPtr;
}

void QIRGenerator::emitGate(const GateInfo* gate) {
    if (!gate) return;
    
    switch (gate->type) {
        case GateType::HADAMARD:
            emitHadamard(gate);
            break;
        case GateType::PAULI_X:
            emitPauliX(gate);
            break;
        case GateType::PAULI_Z:
            emitPauliZ(gate);
            break;
        case GateType::CX:
            emitCNOT(gate);
            break;
        case GateType::MEASURE:
            emitMeasure(gate);
            break;
        case GateType::GATE_BARRIER:
            emitBarrier();
            break;
        default:
            llvm::errs() << "Warning: Unknown gate type\n";
            break;
    }
}

void QIRGenerator::emitHadamard(const GateInfo* gate) {
    if (gate->num_qubits < 1) return;
    
    llvm::Value* qubit = getQubitPtr(gate->qubits_used[0]);
    builder_.CreateCall(hFunc_, {qubit});
}

void QIRGenerator::emitPauliX(const GateInfo* gate) {
    if (gate->num_qubits < 1) return;
    
    llvm::Value* qubit = getQubitPtr(gate->qubits_used[0]);
    builder_.CreateCall(xFunc_, {qubit});
}

void QIRGenerator::emitPauliZ(const GateInfo* gate) {
    if (gate->num_qubits < 1) return;
    
    llvm::Value* qubit = getQubitPtr(gate->qubits_used[0]);
    builder_.CreateCall(zFunc_, {qubit});
}

void QIRGenerator::emitCNOT(const GateInfo* gate) {
    if (gate->num_qubits < 2) return;
    
    llvm::Value* control = getQubitPtr(gate->qubits_used[0]);
    llvm::Value* target = getQubitPtr(gate->qubits_used[1]);
    builder_.CreateCall(cnotFunc_, {control, target});
}

void QIRGenerator::emitMeasure(const GateInfo* gate) {
    if (gate->num_qubits < 1) return;
    
    llvm::Value* qubit = getQubitPtr(gate->qubits_used[0]);
    llvm::Value* result = getResultPtr(gate->classical_bit);
    builder_.CreateCall(measureFunc_, {qubit, result});
}

void QIRGenerator::emitBarrier() {
    // Barrier is a no-op in QIR, but can emit a metadata marker
    // For now, do nothing
}

void QIRGenerator::emitParallelGroup(const ParallelGroup* group, DependencyGraph* graph) {
    if (!group || !graph) return;
    
    for (int i = 0; i < group->num_gates; i++) {
        int gate_id = group->gate_ids[i];
        if (gate_id >= 0 && gate_id < graph->num_gates) {
            emitGate(graph->gates[gate_id]);
        }
    }
}

void QIRGenerator::emitSequentialGate(const GateInfo* gate, int gate_id) {
    emitGate(gate);
}

void QIRGenerator::addOptimizationPasses() {
    // Create pass manager
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    
    // Register analysis passes
    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    
    // Add optimization passes based on level
    llvm::ModulePassManager MPM;
    
    switch (opt_level_) {
        case 1:
            MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
            break;
        case 2:
            MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
            break;
        case 3:
            MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
            break;
        default:
            return;
    }
    
    MPM.run(*module_, MAM);
}

bool QIRGenerator::verify() const {
    std::string error;
    llvm::raw_string_ostream error_stream(error);
    
    if (llvm::verifyModule(*module_, &error_stream)) {
        llvm::errs() << "Module verification failed:\n" << error_stream.str();
        return false;
    }
    
    return true;
}

void QIRGenerator::writeToFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream out(filename, EC);
    
    if (EC) {
        llvm::errs() << "Error opening file " << filename << ": " << EC.message() << "\n";
        return;
    }
    
    module_->print(out, nullptr);
}

void QIRGenerator::writeToStdOut() {
    module_->print(llvm::outs(), nullptr);
}

} // namespace qadvisor
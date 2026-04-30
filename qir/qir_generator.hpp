#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

#include "qubit_manager.hpp"
#include "type_manager.hpp"
#include "metadata_builder.hpp"

#include "../lexer/ast.hpp"
#include "../dependecy/dependency.hpp"
#include "../dependecy/scheduler.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace qadvisor {

/**
 * QIR Generator - Converts quantum circuit AST to LLVM IR
 * following Microsoft's Quantum Intermediate Representation (QIR)
 * specification.
 */
class QIRGenerator {
public:
    /**
     * Constructor
     * @param module_name Name of the LLVM module
     * @param opt_level Optimization level (0, 1, 2, 3)
     */
    QIRGenerator(const std::string& module_name, int opt_level = 2);
    
    /**
     * Destructor
     */
    ~QIRGenerator();
    
    /**
     * Generate QIR from Phase 1 AST and Phase 2 schedule
     * @param ast Phase 1 abstract syntax tree
     * @param schedule Phase 2 parallel schedule
     * @param graph Phase 2 dependency graph
     * @return true if generation succeeded
     */
    bool generate(ProgramAST* ast, Schedule* schedule, DependencyGraph* graph);
    
    /**
     * Write generated IR to file
     * @param filename Output file path
     */
    void writeToFile(const std::string& filename);
    
    /**
     * Write generated IR to standard output
     */
    void writeToStdOut();
    
    /**
     * Get LLVM module (for testing)
     */
    llvm::Module* getModule() const { return module_; }
    
    /**
     * Verify generated IR
     * @return true if IR is valid
     */
    bool verify() const;

private:
    // LLVM components
    std::unique_ptr<llvm::LLVMContext> context_;
    llvm::Module* module_;
    llvm::IRBuilder<> builder_;
    
    // Managers
    std::unique_ptr<TypeManager> type_manager_;
    std::unique_ptr<QubitManager> qubit_manager_;
    std::unique_ptr<MetadataBuilder> metadata_builder_;
    
    // Circuit info
    int num_qubits_;
    int num_classical_bits_;
    std::map<int, llvm::Value*> classical_results_;  // bit index -> Result*
    
    // Optimization level
    int opt_level_;
    
    // Internal methods
    void declareQuantumFunctions();
    void declareRuntimeFunctions();
    void createMainFunction();
    void createEntryBlock();
    void createReturnBlock();
    
    void emitGate(const GateInfo* gate);
    void emitHadamard(const GateInfo* gate);
    void emitPauliX(const GateInfo* gate);
    void emitPauliZ(const GateInfo* gate);
    void emitCNOT(const GateInfo* gate);
    void emitMeasure(const GateInfo* gate);
    void emitBarrier();
    
    void emitParallelGroup(const ParallelGroup* group, DependencyGraph* graph);
    void emitSequentialGate(const GateInfo* gate, int gate_id);
    
    llvm::Value* getQubitPtr(int qubit_index);
    llvm::Value* getResultPtr(int classical_bit);
    
    void addOptimizationPasses();
    
    // Gate functions (declarations)
    llvm::Function* hFunc_;
    llvm::Function* xFunc_;
    llvm::Function* zFunc_;
    llvm::Function* cnotFunc_;
    llvm::Function* measureFunc_;
    llvm::Function* readResultFunc_;
};

} // namespace qadvisor


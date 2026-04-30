#include "qubit_manager.hpp"

namespace qadvisor {

QubitManager::QubitManager(int num_qubits, llvm::LLVMContext& context, llvm::IRBuilder<>& builder)
    : num_qubits_(num_qubits)
    , context_(context)
    , builder_(builder)
    , qubit_type_(nullptr)
    , qubit_ptrs_(num_qubits, nullptr)
    , allocated_(num_qubits, false)
{
    // Create opaque qubit type
    qubit_type_ = llvm::StructType::create(context_, "Qubit");
}

QubitManager::~QubitManager() {
    // Nothing to clean up - LLVM handles memory
}

void QubitManager::allocateAll() {
    // In QIR Base Profile, qubits are statically allocated and casted using inttoptr.
    // No dynamic alloca is needed at the start of the function for base profile.
}

llvm::Value* QubitManager::getQubitPtr(int index) {
    if (index < 0 || index >= num_qubits_) {
        llvm::errs() << "Error: Qubit index " << index << " out of range (0-" 
                     << num_qubits_ - 1 << ")\n";
        return nullptr;
    }
    
    // Use standard QIR integer casting: inttoptr i64 %index to %Qubit*
    llvm::Type* int64_type = llvm::Type::getInt64Ty(context_);
    llvm::Value* int_val = llvm::ConstantInt::get(int64_type, index);
    
    llvm::Value* ptr = builder_.CreateIntToPtr(int_val, getQubitPtrType(), "qubit_" + std::to_string(index));
    allocated_[index] = true;
    return ptr;
}

bool QubitManager::isAllocated(int index) const {
    return index >= 0 && index < num_qubits_ && allocated_[index];
}

llvm::PointerType* QubitManager::getQubitPtrType() const {
    return llvm::PointerType::get(qubit_type_, 0);
}

} // namespace qadvisor
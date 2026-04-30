#include "type_manager.hpp"

namespace qadvisor {

TypeManager::TypeManager(llvm::LLVMContext& context)
    : context_(context)
    , qubit_type_(nullptr)
    , result_type_(nullptr)
{
}

TypeManager::~TypeManager() {
    // LLVM manages memory
}

llvm::Type* TypeManager::getVoidType() const {
    return llvm::Type::getVoidTy(context_);
}

llvm::Type* TypeManager::getInt1Type() const {
    return llvm::Type::getInt1Ty(context_);
}

llvm::Type* TypeManager::getInt8Type() const {
    return llvm::Type::getInt8Ty(context_);
}

llvm::Type* TypeManager::getInt32Type() const {
    return llvm::Type::getInt32Ty(context_);
}

llvm::Type* TypeManager::getInt64Type() const {
    return llvm::Type::getInt64Ty(context_);
}

llvm::StructType* TypeManager::getQubitType() const {
    if (!qubit_type_) {
        qubit_type_ = llvm::StructType::create(context_, "Qubit");
    }
    return qubit_type_;
}

llvm::PointerType* TypeManager::getQubitPtrType() const {
    return llvm::PointerType::get(getQubitType(), 0);
}

llvm::StructType* TypeManager::getResultType() const {
    if (!result_type_) {
        result_type_ = llvm::StructType::create(context_, "Result");
    }
    return result_type_;
}

llvm::PointerType* TypeManager::getResultPtrType() const {
    return llvm::PointerType::get(getResultType(), 0);
}

llvm::ArrayType* TypeManager::getArrayType(llvm::Type* element_type, unsigned size) {
    return llvm::ArrayType::get(element_type, size);
}

} // namespace qadvisor
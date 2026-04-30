#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

namespace qadvisor {

/**
 * Type Manager - Manages LLVM types used in QIR generation.
 * Provides consistent type definitions across the generator.
 */
class TypeManager {
public:
    /**
     * Constructor
     * @param context LLVM context
     */
    explicit TypeManager(llvm::LLVMContext& context);
    
    /**
     * Destructor
     */
    ~TypeManager();
    
    // Basic types
    llvm::Type* getVoidType() const;
    llvm::Type* getInt1Type() const;   // i1 (boolean)
    llvm::Type* getInt8Type() const;   // i8 (byte)
    llvm::Type* getInt32Type() const;  // i32 (integer)
    llvm::Type* getInt64Type() const;  // i64 (size_t)
    
    // Quantum types
    llvm::StructType* getQubitType() const;
    llvm::PointerType* getQubitPtrType() const;
    
    llvm::StructType* getResultType() const;
    llvm::PointerType* getResultPtrType() const;
    
    // Array types (for qubit/classical arrays)
    llvm::ArrayType* getArrayType(llvm::Type* element_type, unsigned size);
    
private:
    llvm::LLVMContext& context_;
    
    // Lazy-initialized types
    mutable llvm::StructType* qubit_type_;
    mutable llvm::StructType* result_type_;
};

} // namespace qadvisor


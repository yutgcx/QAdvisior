#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

#include <vector>
#include <map>

namespace qadvisor {

/**
 * Qubit Manager - Handles qubit allocation and tracking in LLVM IR.
 * Qubits are represented as opaque pointers in QIR.
 */
class QubitManager {
public:
    /**
     * Constructor
     * @param num_qubits Total number of qubits in the circuit
     * @param context LLVM context
     * @param builder LLVM IR builder
     */
    QubitManager(int num_qubits, llvm::LLVMContext& context, llvm::IRBuilder<>& builder);
    
    /**
     * Destructor
     */
    ~QubitManager();
    
    /**
     * Allocate all qubits at the start of the function
     */
    void allocateAll();
    
    /**
     * Get pointer to a specific qubit
     * @param index Qubit index (0-based)
     * @return LLVM Value* pointing to the qubit
     */
    llvm::Value* getQubitPtr(int index);
    
    /**
     * Check if qubit is allocated
     * @param index Qubit index
     * @return true if allocated
     */
    bool isAllocated(int index) const;
    
    /**
     * Get total number of qubits
     */
    int getNumQubits() const { return num_qubits_; }
    
    /**
     * Get LLVM type for qubit pointer
     */
    llvm::PointerType* getQubitPtrType() const;

private:
    int num_qubits_;
    llvm::LLVMContext& context_;
    llvm::IRBuilder<>& builder_;
    llvm::StructType* qubit_type_;
    std::vector<llvm::AllocaInst*> qubit_ptrs_;
    std::vector<bool> allocated_;
};

} // namespace qadvisor


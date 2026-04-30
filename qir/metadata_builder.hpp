#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

#include <string>
#include <optional>

namespace qadvisor {

/**
 * Metadata Builder - Creates LLVM metadata for QIR.
 * Used to annotate parallel groups, source locations, etc.
 */
class MetadataBuilder {
public:
    /**
     * Constructor
     * @param context LLVM context
     */
    explicit MetadataBuilder(llvm::LLVMContext& context);
    
    /**
     * Destructor
     */
    ~MetadataBuilder();
    
    /**
     * Add parallel group metadata to an instruction
     * @param inst LLVM instruction
     * @param group_id Parallel group identifier
     */
    void addParallelMetadata(llvm::Instruction* inst, int group_id);
    
    /**
     * Add source location metadata
     * @param inst LLVM instruction
     * @param filename Source file name
     * @param line Line number
     * @param column Column number
     */
    void addSourceLocation(llvm::Instruction* inst, 
                          const std::string& filename,
                          int line, int column);
    
    /**
     * Add quantum operation metadata (for debugging/analysis)
     * @param inst LLVM instruction
     * @param gate_type Type of quantum gate
     */
    void addQuantumOpMetadata(llvm::Instruction* inst, const std::string& gate_type);
    
    /**
     * Get module-level metadata for QIR
     */
    void attachModuleMetadata(llvm::Module* module);

private:
    llvm::LLVMContext& context_;
    llvm::MDNode* parallel_md_kind_;
};

} // namespace qadvisor


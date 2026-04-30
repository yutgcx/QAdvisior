#include "metadata_builder.hpp"

using namespace llvm;

namespace qadvisor {

MetadataBuilder::MetadataBuilder(LLVMContext& context)
    : context_(context)
    , parallel_md_kind_(nullptr)
{
    // Register metadata kinds
    unsigned parallel_kind = context.getMDKindID("quantum.parallel");
    parallel_md_kind_ = MDNode::get(context, {});
}

MetadataBuilder::~MetadataBuilder() {
}

void MetadataBuilder::addParallelMetadata(Instruction* inst, int group_id) {
    if (!inst) return;
    
    // Create metadata node with group ID
    Metadata* vals[] = {
        MDString::get(context_, "parallel_group"),
        ConstantAsMetadata::get(ConstantInt::get(
            IntegerType::get(context_, 32), group_id))
    };
    
    MDNode* md = MDNode::get(context_, vals);
    inst->setMetadata("quantum.parallel", md);
}

void MetadataBuilder::addSourceLocation(Instruction* inst,
                                        const std::string& filename,
                                        int line, int column) {
    if (!inst) return;
    
    // Create debug location
    // (Simplified - full implementation would create DILocation)
}

void MetadataBuilder::addQuantumOpMetadata(Instruction* inst, const std::string& gate_type) {
    if (!inst) return;
    
    Metadata* vals[] = {
        MDString::get(context_, "quantum.op"),
        MDString::get(context_, gate_type)
    };
    
    MDNode* md = MDNode::get(context_, vals);
    inst->setMetadata("quantum.op", md);
}

void MetadataBuilder::attachModuleMetadata(Module* module) {
    if (!module) return;
    
    // Add QIR metadata properties
    module->addModuleFlag(Module::Error, "qir_major_version", uint32_t(1));
    module->addModuleFlag(Module::Max, "qir_minor_version", uint32_t(0));
    module->addModuleFlag(Module::Warning, "qir_profiles", MDString::get(context_, "base_profile"));
}

} // namespace qadvisor
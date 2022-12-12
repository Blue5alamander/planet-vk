#include <planet/vk/device.hpp>
#include <planet/vk/shader_module.hpp>


/**
 * ## planet::vk::shader_module
 */


planet::vk::shader_module::shader_module(
        vk::device const &d, std::vector<std::byte> bc)
: device{d}, spirv{std::move(bc)} {
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = spirv.size();
    info.pCode = reinterpret_cast<std::uint32_t *>(spirv.data());
    handle.create<vkCreateShaderModule>(device.get(), info);
}


VkPipelineShaderStageCreateInfo planet::vk::shader_module::shader_stage_info(
        VkShaderStageFlagBits const flags, char const *entry) {
    VkPipelineShaderStageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = flags;
    info.module = handle.get();
    info.pName = entry;
    return info;
}

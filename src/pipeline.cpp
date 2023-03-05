#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/pipeline.hpp>


/// ## `planet::vk::pipeline_layout`


planet::vk::pipeline_layout::pipeline_layout(vk::device &d) : device{d} {
    VkPipelineLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    handle.create<vkCreatePipelineLayout>(device.get(), info);
}


planet::vk::pipeline_layout::pipeline_layout(
        vk::device &d, descriptor_set_layout const &dsl)
: device{d} {
    VkPipelineLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = dsl.address();
    handle.create<vkCreatePipelineLayout>(device.get(), info);
}


planet::vk::pipeline_layout::pipeline_layout(
        vk::device &d, VkPipelineLayoutCreateInfo const &info)
: device{d} {
    handle.create<vkCreatePipelineLayout>(device.get(), info);
}


/// ## `planet::vk::graphics_pipeline`


planet::vk::graphics_pipeline::graphics_pipeline(
        vk::device &d,
        VkGraphicsPipelineCreateInfo &info,
        vk::render_pass rp,
        pipeline_layout pl)
: device{d}, render_pass{std::move(rp)}, layout{std::move(pl)} {
    info.renderPass = render_pass.get();
    info.layout = layout.get();

    VkPipeline ph = VK_NULL_HANDLE;
    planet::vk::worked(vkCreateGraphicsPipelines(
            device.get(), VK_NULL_HANDLE, 1, &info, nullptr, &ph));
    handle = handle_type::bind(device.get(), ph);
}

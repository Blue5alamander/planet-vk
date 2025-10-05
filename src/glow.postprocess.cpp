#include <planet/functional.hpp>
#include <planet/vk/engine/postprocess/glow.hpp>
#include <planet/vk/engine/renderer.hpp>


using namespace std::literals;


/// ## `planet::vk::engine::pipeline::postprocess`


planet::vk::engine::postprocess::glow::glow(parameters p)
: sampler_layout{p.renderer.app.device, VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr}},
  sampler{
          {.device = p.renderer.app.device,
           .address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE}},
  descriptor_pool{
          p.renderer.app.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<std::uint32_t>(max_frames_in_flight)},
  descriptor_sets{descriptor_pool, sampler_layout, max_frames_in_flight},
  pipeline{create_graphics_pipeline(
          {.app = p.renderer.app,
           .renderer = p.renderer,
           .vertex_shader = {"planet-vk-engine/postprocess.vert.spirv"sv},
           .fragment_shader = {"planet-vk-engine/postprocess.copy.frag.spirv"sv},
           .binding_descriptions = {},
           .attribute_descriptions = {},
           .colour_attachments = 1,
           .render_pass = p.renderer.present_render_pass,
           .write_to_depth_buffer = false,
           .multisampling = VK_SAMPLE_COUNT_1_BIT,
           .blend_mode = blend_mode::none,
           .pipeline_layout =
                   pipeline_layout{p.renderer.app.device, sampler_layout}})} {}


void planet::vk::engine::postprocess::glow::update_descriptors(
        engine::renderer &renderer) {
    by_index(max_frames_in_flight, [&](std::size_t const index) {
        VkDescriptorImageInfo image_info = {};
        image_info.sampler = sampler.get();
        image_info.imageView = renderer.scene_colours[index].image_view.get();
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptor_sets[index];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image_info;

        vkUpdateDescriptorSets(
                renderer.app.device.get(), 1, &write, 0, nullptr);
    });
}


void planet::vk::engine::postprocess::glow::render_subpass(
        render_parameters rp, std::uint32_t const image_index) {
    VkRenderPassBeginInfo present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    present_info.renderPass = rp.renderer.present_render_pass.get();
    present_info.framebuffer =
            rp.renderer.swap_chain.frame_buffers[image_index].get();
    present_info.renderArea.offset = {0, 0};
    present_info.renderArea.extent = rp.renderer.swap_chain.extents;
    VkClearValue clear = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    present_info.clearValueCount = 1;
    present_info.pClearValues = &clear;

    vkCmdBeginRenderPass(
            rp.cb.get(), &present_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport (same as scene)
    VkViewport viewport = {
            0.0f,
            static_cast<float>(rp.renderer.app.window.height()),
            static_cast<float>(rp.renderer.app.window.width()),
            -static_cast<float>(rp.renderer.app.window.height()),
            0.0f,
            1.0f};
    vkCmdSetViewport(rp.cb.get(), 0, 1, &viewport);

    /// Bind and draw copy pipeline (full-screen triangle)
    vkCmdBindPipeline(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get());
    VkDescriptorSet ds = descriptor_sets[rp.current_frame];
    vkCmdBindDescriptorSets(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout.get(),
            0, 1, &ds, 0, nullptr);
    vkCmdDraw(
            rp.cb.get(), 3, 1, 0,
            0); // 3 verts, no instance/vertex/index buffer

    vkCmdEndRenderPass(rp.cb.get());
}

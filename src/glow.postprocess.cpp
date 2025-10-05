#include <planet/functional.hpp>
#include <planet/vk/engine/postprocess/glow.hpp>
#include <planet/vk/engine/renderer.hpp>


using namespace std::literals;


/// ## `planet::vk::engine::pipeline::postprocess`


planet::vk::engine::postprocess::glow::glow(parameters p)
: app{p.renderer.app},
  renderer{p.renderer},
  input_attachments{array_of<max_frames_in_flight>([this]() {
      return engine::colour_attachment{
              {.allocator = renderer.per_swap_chain_memory,
               .extents = renderer.swap_chain.extents,
               .format = renderer.swap_chain.image_format,
               .usage_flags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT}};
  })},
  input_colours{array_of<max_frames_in_flight>([this]() {
      return engine::colour_attachment{
              {.allocator = renderer.per_swap_chain_memory,
               .extents = renderer.swap_chain.extents,
               .format = renderer.swap_chain.image_format,
               .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT}};
  })},
  sampler_layout{
          p.renderer.app.device,
          VkDescriptorSetLayoutBinding{
                  .binding = 0,
                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                  .descriptorCount = 1,
                  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .pImmutableSamplers = nullptr}},
  sampler{
          {.device = p.renderer.app.device,
           .address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE}},
  descriptor_pool{
          p.renderer.app.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<std::uint32_t>(max_frames_in_flight)},
  descriptor_sets{descriptor_pool, sampler_layout, max_frames_in_flight},
  present_render_pass{[this]() {
      auto attachments =
              std::array{renderer.swap_chain.attachment_description()};
      VkAttachmentReference colour_ref{
              .attachment = 0,
              .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colour_ref;

      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkRenderPassCreateInfo render_pass_info = {};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      render_pass_info.attachmentCount = attachments.size();
      render_pass_info.pAttachments = attachments.data();
      render_pass_info.subpassCount = 1;
      render_pass_info.pSubpasses = &subpass;
      render_pass_info.dependencyCount = 1;
      render_pass_info.pDependencies = &dependency;

      return vk::render_pass{app.device, render_pass_info};
  }()},
  pipeline{create_graphics_pipeline(
          {.app = app,
           .renderer = renderer,
           .vertex_shader = {"planet-vk-engine/postprocess.vert.spirv"sv},
           .fragment_shader = {"planet-vk-engine/postprocess.copy.frag.spirv"sv},
           .binding_descriptions = {},
           .attribute_descriptions = {},
           .colour_attachments = 1,
           .render_pass = present_render_pass,
           .write_to_depth_buffer = false,
           .multisampling = VK_SAMPLE_COUNT_1_BIT,
           .blend_mode = blend_mode::none,
           .pipeline_layout =
                   pipeline_layout{renderer.app.device, sampler_layout}})} {}


void planet::vk::engine::postprocess::glow::update_descriptors() {
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
    present_info.renderPass = present_render_pass.get();
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

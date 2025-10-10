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
               .usage_flags = static_cast<VkImageUsageFlagBits>(
                       VK_IMAGE_USAGE_SAMPLED_BIT
                       | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
               .sample_count = VK_SAMPLE_COUNT_1_BIT}};
  })},
  downsized_input{array_of<max_frames_in_flight>([this]() {
      return engine::colour_attachment{
              {.allocator = renderer.per_swap_chain_memory,
               .extents =
                       {.width = renderer.swap_chain.extents.width / 2,
                        .height = renderer.swap_chain.extents.height / 2},
               .format = renderer.swap_chain.image_format,
               .usage_flags = static_cast<VkImageUsageFlagBits>(
                       VK_IMAGE_USAGE_SAMPLED_BIT
                       | VK_IMAGE_USAGE_TRANSFER_DST_BIT)}};
  })},
  present_sampler_layout{
          p.renderer.app.device,
          VkDescriptorSetLayoutBinding{
                  .binding = 0,
                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                  .descriptorCount = 1,
                  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .pImmutableSamplers = nullptr}},
  present_sampler{
          {.device = p.renderer.app.device,
           .address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE}},
  present_descriptor_pool{
          p.renderer.app.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<std::uint32_t>(max_frames_in_flight)},
  present_descriptor_sets{
          present_descriptor_pool, present_sampler_layout,
          max_frames_in_flight},
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
  present_pipeline{create_graphics_pipeline(
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
           .pipeline_layout = pipeline_layout{
                   renderer.app.device, present_sampler_layout}})} {
    /// ### Initial image transitions
    auto barriers = array_of<max_frames_in_flight>([&](auto const index) {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = downsized_input[index].image.get();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        return barrier;
    });
    /**
     * When we use the glow effect image it needs to transition between states
     * where it can be used as a target for the bitblit (that reduces the size),
     * and where it can be used as a texture with a sampler. To get this to work
     * properly we do a one off transition here that sets up as it would be at
     * the end of a frame. That stops errors when the image is used for the
     * first time where the creation format doesn't match the one it'll have at
     * the end of the frame render loop.
     */
    auto cb = planet::vk::command_buffer::single_use(renderer.command_pool);
    vkCmdPipelineBarrier(
            cb.get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
            barriers.size(), barriers.data());
    cb.end_and_submit();
}


void planet::vk::engine::postprocess::glow::update_descriptors() {
    by_index(max_frames_in_flight, [&](std::size_t const index) {
        VkDescriptorImageInfo image_info = {};
        image_info.sampler = present_sampler.get();
        image_info.imageView = renderer.scene_colours[index].image_view.get();
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = present_descriptor_sets[index];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image_info;

        vkUpdateDescriptorSets(
                renderer.app.device.get(), 1, &write, 0, nullptr);
    });
}


/// ### `render_subpass`
void planet::vk::engine::postprocess::glow::render_subpass(
        render_parameters rp, std::uint32_t const image_index) {
    /// #### Downsample the `input_colours` to half size
    auto &input_image = input_colours[rp.current_frame].image;
    auto &downsized_image = downsized_input[rp.current_frame].image;

    VkImageMemoryBarrier src_barrier = {};
    src_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    src_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    src_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    src_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    src_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    src_barrier.image = input_image.get();
    src_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    src_barrier.subresourceRange.baseMipLevel = 0;
    src_barrier.subresourceRange.levelCount = 1;
    src_barrier.subresourceRange.baseArrayLayer = 0;
    src_barrier.subresourceRange.layerCount = 1;
    src_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    src_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
            rp.cb.get(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &src_barrier);

    VkImageMemoryBarrier dst_barrier = {};
    dst_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    dst_barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dst_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dst_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dst_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dst_barrier.image = downsized_image.get();
    dst_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    dst_barrier.subresourceRange.baseMipLevel = 0;
    dst_barrier.subresourceRange.levelCount = 1;
    dst_barrier.subresourceRange.baseArrayLayer = 0;
    dst_barrier.subresourceRange.layerCount = 1;
    dst_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dst_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
            rp.cb.get(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &dst_barrier);

    VkImageBlit blit_region = {};
    blit_region.srcOffsets[0] = {0, 0, 0};
    blit_region.srcOffsets[1] = {
            static_cast<std::int32_t>(input_image.width),
            static_cast<std::int32_t>(input_image.height), 1};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = 0;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;

    blit_region.dstOffsets[0] = {0, 0, 0};
    blit_region.dstOffsets[1] = {
            static_cast<std::int32_t>(downsized_image.width),
            static_cast<std::int32_t>(downsized_image.height), 1};
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = 0;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;

    vkCmdBlitImage(
            rp.cb.get(), input_image.get(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, downsized_image.get(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region,
            VK_FILTER_LINEAR);

    dst_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dst_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dst_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dst_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
            rp.cb.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &dst_barrier);


    /// #### Perform vertical and horizontal blur


    /// #### Composite the two images together

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
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            present_pipeline.get());
    VkDescriptorSet ds = present_descriptor_sets[rp.current_frame];
    vkCmdBindDescriptorSets(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            present_pipeline.layout.get(), 0, 1, &ds, 0, nullptr);
    vkCmdDraw(
            rp.cb.get(), 3, 1, 0,
            0); // 3 verts, no instance/vertex/index buffer

    vkCmdEndRenderPass(rp.cb.get());
}

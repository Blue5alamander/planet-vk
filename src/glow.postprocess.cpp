#include <planet/functional.hpp>
#include <planet/vk/engine/postprocess/glow.hpp>
#include <planet/vk/engine/renderer.hpp>


using namespace std::literals;


/// ## `planet::vk::engine::pipeline::postprocess`


planet::vk::engine::postprocess::glow::glow(parameters p)
: app{p.renderer.app},
  renderer{p.renderer},
  input_attachments{
          {.allocator = renderer.per_swap_chain_memory,
           .extents = renderer.swap_chain.extents,
           .format = renderer.swap_chain.image_format,
           .usage_flags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT}},
  input_colours{
          {.allocator = renderer.per_swap_chain_memory,
           .extents = renderer.swap_chain.extents,
           .format = renderer.swap_chain.image_format,
           .usage_flags = static_cast<VkImageUsageFlagBits>(
                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
           .sample_count = VK_SAMPLE_COUNT_1_BIT}},
  downsized_input{
          {.allocator = renderer.per_swap_chain_memory,
           .extents =
                   {.width = renderer.swap_chain.extents.width / 2,
                    .height = renderer.swap_chain.extents.height / 2},
           .format = renderer.swap_chain.image_format,
           .usage_flags = static_cast<VkImageUsageFlagBits>(
                   VK_IMAGE_USAGE_SAMPLED_BIT
                   | VK_IMAGE_USAGE_TRANSFER_DST_BIT)}},
  horizontal_blur{
          {.allocator = renderer.per_swap_chain_memory,
           .extents =
                   {.width = renderer.swap_chain.extents.width / 2,
                    .height = renderer.swap_chain.extents.height / 2},
           .format = renderer.swap_chain.image_format,
           .usage_flags = static_cast<VkImageUsageFlagBits>(
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                   | VK_IMAGE_USAGE_SAMPLED_BIT)}},
  vertical_blur{
          {.allocator = renderer.per_swap_chain_memory,
           .extents =
                   {.width = renderer.swap_chain.extents.width / 2,
                    .height = renderer.swap_chain.extents.height / 2},
           .format = renderer.swap_chain.image_format,
           .usage_flags = static_cast<VkImageUsageFlagBits>(
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                   | VK_IMAGE_USAGE_SAMPLED_BIT)}},
  blur_sampler_layout{
          p.renderer.app.device,
          VkDescriptorSetLayoutBinding{
                  .binding = 0,
                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                  .descriptorCount = 1,
                  .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .pImmutableSamplers = nullptr}},
  blur_sampler{
          {.device = p.renderer.app.device,
           .address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE}},
  blur_descriptor_pool{
          p.renderer.app.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<std::uint32_t>(
                  2 * max_frames_in_flight)}, // For both horizontal and
                                              // vertical sets
  horizontal_descriptor_sets{
          blur_descriptor_pool, blur_sampler_layout, max_frames_in_flight},
  vertical_descriptor_sets{
          blur_descriptor_pool, blur_sampler_layout, max_frames_in_flight},
  blur_render_pass{[this]() {
      VkAttachmentDescription attachment = {};
      attachment.format = renderer.swap_chain.image_format;
      attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

      VkAttachmentReference colour_ref{
              .attachment = 0,
              .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colour_ref;

      std::array<VkSubpassDependency, 2> dependencies = {};
      dependencies[0] = {};
      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      dependencies[0].dstStageMask =
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = 0;
      dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      dependencies[1] = {};
      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask =
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      VkRenderPassCreateInfo render_pass_info = {};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      render_pass_info.attachmentCount = 1;
      render_pass_info.pAttachments = &attachment;
      render_pass_info.subpassCount = 1;
      render_pass_info.pSubpasses = &subpass;
      render_pass_info.dependencyCount =
              static_cast<std::uint32_t>(dependencies.size());
      render_pass_info.pDependencies = dependencies.data();

      return vk::render_pass{app.device, render_pass_info};
  }()},
  horizontal_pipeline{create_graphics_pipeline(
          {.app = app,
           .renderer = renderer,
           .vertex_shader = {"planet-vk-engine/postprocess.vert.spirv"sv},
           .fragment_shader =
                   {"planet-vk-engine/postprocess.blur.frag.horizontal.spirv"sv},
           .binding_descriptions = {},
           .attribute_descriptions = {},
           .colour_attachments = 1,
           .render_pass = blur_render_pass,
           .write_to_depth_buffer = false,
           .multisampling = VK_SAMPLE_COUNT_1_BIT,
           .blend_mode = blend_mode::none,
           .pipeline_layout =
                   pipeline_layout{renderer.app.device, blur_sampler_layout}})},
  vertical_pipeline{create_graphics_pipeline(
          {.app = app,
           .renderer = renderer,
           .vertex_shader = {"planet-vk-engine/postprocess.vert.spirv"sv},
           .fragment_shader =
                   {"planet-vk-engine/postprocess.blur.frag.vertical.spirv"sv},
           .binding_descriptions = {},
           .attribute_descriptions = {},
           .colour_attachments = 1,
           .render_pass = blur_render_pass,
           .write_to_depth_buffer = false,
           .multisampling = VK_SAMPLE_COUNT_1_BIT,
           .blend_mode = blend_mode::none,
           .pipeline_layout =
                   pipeline_layout{renderer.app.device, blur_sampler_layout}})},
  horizontal_frame_buffers{
          array_of<max_frames_in_flight>([&](auto const index) {
              auto &img = horizontal_blur.image[index];
              std::array attachments{horizontal_blur.image_view[index].get()};
              return vk::frame_buffer{
                      app.device,
                      {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                       .pNext = nullptr,
                       .flags = {},
                       .renderPass = blur_render_pass.get(),
                       .attachmentCount = attachments.size(),
                       .pAttachments = attachments.data(),
                       .width = img.width,
                       .height = img.height,
                       .layers = 1}};
          })},
  vertical_frame_buffers{array_of<max_frames_in_flight>([&](auto const index) {
      auto &img = vertical_blur.image[index];
      std::array attachments{vertical_blur.image_view[index].get()};
      return vk::frame_buffer{
              app.device,
              {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
               .pNext = nullptr,
               .flags = {},
               .renderPass = blur_render_pass.get(),
               .attachmentCount = attachments.size(),
               .pAttachments = attachments.data(),
               .width = img.width,
               .height = img.height,
               .layers = 1}};
  })},
  present_sampler_layout{
          p.renderer.app.device,
          std::array{
                  VkDescriptorSetLayoutBinding{
                          .binding = 0,
                          .descriptorType =
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                          .descriptorCount = 1,
                          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                          .pImmutableSamplers = nullptr},
                  VkDescriptorSetLayoutBinding{
                          .binding = 1,
                          .descriptorType =
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                          .descriptorCount = 1,
                          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                          .pImmutableSamplers = nullptr}}},
  present_sampler{
          {.device = p.renderer.app.device,
           .address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE}},
  present_descriptor_pool{
          p.renderer.app.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<std::uint32_t>(max_frames_in_flight * 2)},
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
           .fragment_shader = {"planet-vk-engine/postprocess.glow.frag.spirv"sv},
           .binding_descriptions = {},
           .attribute_descriptions = {},
           .colour_attachments = 1,
           .render_pass = present_render_pass,
           .write_to_depth_buffer = false,
           .multisampling = VK_SAMPLE_COUNT_1_BIT,
           .blend_mode = blend_mode::none,
           .pipeline_layout = pipeline_layout{
                   renderer.app.device, present_sampler_layout}})} {
    initial_image_transition();
    update_descriptors();
}


void planet::vk::engine::postprocess::glow::initial_image_transition() {
    /// ### Initial image transitions
    felspar::memory::small_vector<VkImageMemoryBarrier, max_frames_in_flight * 2>
            barriers;
    auto const barriers_for = [&](auto &images) {
        by_index(max_frames_in_flight, [&](auto const index) {
            barriers.push_back(images.image[index].transition(
                    {.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     .destination_access_mask = VK_ACCESS_SHADER_READ_BIT}));
        });
    };
    barriers_for(horizontal_blur);
    barriers_for(vertical_blur);
    /**
     * When we use the glow effect image it needs to transition between states
     * where it can be used as a target for the bitblit (that reduces the size),
     * and where it can be used as a texture with a sampler. To get this to work
     * properly we do a one off transition here that sets up as it would be at
     * the end of a frame. That stops errors when the image is used for the
     * first time where the creation format doesn't match the one it'll have at
     * the end of the frame render loop.
     *
     * The `downsized_image` doesn't need a transition because all transitions
     * are explicit in the code below, so the `image`'s default tracking of
     * layouts works fine.
     */
    planet::vk::command_buffer::single_use(renderer.command_pool)
            .pipeline_barrier(
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barriers)
            .end_and_submit();
}


/// ### `update_descriptors`
void planet::vk::engine::postprocess::glow::update_descriptors() {
    by_index(max_frames_in_flight, [&](std::size_t const index) {
        /// #### Horizontal blur descriptors (samples from downsized_input)
        auto horizontal_info = VkDescriptorImageInfo{
                .sampler = blur_sampler.get(),
                .imageView = downsized_input.image_view[index].get(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet horizontal_write = {};
        horizontal_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        horizontal_write.dstSet = horizontal_descriptor_sets[index];
        horizontal_write.dstBinding = 0;
        horizontal_write.dstArrayElement = 0;
        horizontal_write.descriptorCount = 1;
        horizontal_write.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        horizontal_write.pImageInfo = &horizontal_info;

        vkUpdateDescriptorSets(
                renderer.app.device.get(), 1, &horizontal_write, 0, nullptr);

        /// #### Vertical blur descriptors (samples from horizontal_blur)
        auto vertical_info = VkDescriptorImageInfo{
                .sampler = blur_sampler.get(),
                .imageView = horizontal_blur.image_view[index].get(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet vertical_write = {};
        vertical_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vertical_write.dstSet = vertical_descriptor_sets[index];
        vertical_write.dstBinding = 0;
        vertical_write.dstArrayElement = 0;
        vertical_write.descriptorCount = 1;
        vertical_write.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        vertical_write.pImageInfo = &vertical_info;

        vkUpdateDescriptorSets(
                renderer.app.device.get(), 1, &vertical_write, 0, nullptr);

        /// #### Composite stage descriptors
        std::array<VkWriteDescriptorSet, 2> write = {};

        auto scene_info = VkDescriptorImageInfo{
                .sampler = present_sampler.get(),
                .imageView = renderer.scene_colours.image_view[index].get(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write[0].dstSet = present_descriptor_sets[index];
        write[0].dstBinding = 0;
        write[0].dstArrayElement = 0;
        write[0].descriptorCount = 1;
        write[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write[0].pImageInfo = &scene_info;

        auto glow_info = VkDescriptorImageInfo{
                .sampler = present_sampler.get(),
                .imageView = vertical_blur.image_view[index].get(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write[1].dstSet = present_descriptor_sets[index];
        write[1].dstBinding = 1;
        write[1].dstArrayElement = 0;
        write[1].descriptorCount = 1;
        write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write[1].pImageInfo = &glow_info;

        vkUpdateDescriptorSets(
                renderer.app.device.get(), write.size(), write.data(), 0,
                nullptr);
    });
}


/// ### `render_subpass`
void planet::vk::engine::postprocess::glow::render_subpass(
        render_parameters rp, std::uint32_t const image_index) {
    /// #### Downsample the `input_colours` to half size
    auto &input_image = input_colours.image[rp.current_frame];
    auto &downsized_image = downsized_input.image[rp.current_frame];

    rp.cb.pipeline_barrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            std::array{input_image.transition(
                    {.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     .new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     .source_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     .destination_access_mask = VK_ACCESS_TRANSFER_READ_BIT})});
    rp.cb.pipeline_barrier(
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            std::array{downsized_image.transition(
                    {.new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     .source_access_mask = VK_ACCESS_SHADER_READ_BIT,
                     .destination_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT})});

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

    rp.cb.pipeline_barrier(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            std::array{downsized_image.transition(
                    {.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     .source_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                     .destination_access_mask = VK_ACCESS_SHADER_READ_BIT})});


    /// #### Perform horizontal then vertical blur

    VkRenderPassBeginInfo horizontal_info = {};
    horizontal_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    horizontal_info.renderPass = blur_render_pass.get();
    horizontal_info.framebuffer =
            horizontal_frame_buffers[rp.current_frame].get();
    horizontal_info.renderArea.offset = {0, 0};
    horizontal_info.renderArea.extent = {
            .width = horizontal_blur.image[rp.current_frame].width,
            .height = horizontal_blur.image[rp.current_frame].height};
    VkClearValue horizontal_clear = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    horizontal_info.clearValueCount = 1;
    horizontal_info.pClearValues = &horizontal_clear;

    vkCmdBeginRenderPass(
            rp.cb.get(), &horizontal_info, VK_SUBPASS_CONTENTS_INLINE);

    auto const half_size = horizontal_blur.image[rp.current_frame].extents();
    VkViewport horizontal_viewport = {
            0.0f, half_size.height, half_size.width, -half_size.height, 0.0f,
            1.0f};
    vkCmdSetViewport(rp.cb.get(), 0, 1, &horizontal_viewport);

    vkCmdBindPipeline(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            horizontal_pipeline.get());
    VkDescriptorSet horizontal_ds =
            horizontal_descriptor_sets[rp.current_frame];
    vkCmdBindDescriptorSets(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            horizontal_pipeline.layout.get(), 0, 1, &horizontal_ds, 0, nullptr);
    vkCmdDraw(rp.cb.get(), 3, 1, 0, 0);

    vkCmdEndRenderPass(rp.cb.get());


    VkRenderPassBeginInfo vertical_info = {};
    vertical_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vertical_info.renderPass = blur_render_pass.get();
    vertical_info.framebuffer = vertical_frame_buffers[rp.current_frame].get();
    vertical_info.renderArea.offset = {0, 0};
    vertical_info.renderArea.extent = {
            .width = vertical_blur.image[rp.current_frame].width,
            .height = vertical_blur.image[rp.current_frame].height};
    VkClearValue vertical_clear = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    vertical_info.clearValueCount = 1;
    vertical_info.pClearValues = &vertical_clear;

    vkCmdBeginRenderPass(
            rp.cb.get(), &vertical_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vertical_viewport = {
            0.0f, half_size.height, half_size.width, -half_size.height, 0.0f,
            1.0f};
    vkCmdSetViewport(rp.cb.get(), 0, 1, &vertical_viewport);

    vkCmdBindPipeline(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            vertical_pipeline.get());
    VkDescriptorSet vertical_ds = vertical_descriptor_sets[rp.current_frame];
    vkCmdBindDescriptorSets(
            rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            vertical_pipeline.layout.get(), 0, 1, &vertical_ds, 0, nullptr);
    vkCmdDraw(rp.cb.get(), 3, 1, 0, 0);

    vkCmdEndRenderPass(rp.cb.get());


    /// #### Composite the two images together

    rp.cb.pipeline_barrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            std::array{renderer.scene_colours.image[rp.current_frame].transition(
                    {.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     .new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     .source_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     .destination_access_mask = VK_ACCESS_SHADER_READ_BIT})});

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

    VkViewport viewport = {
            0.0f,
            static_cast<float>(rp.renderer.app.window.height()),
            static_cast<float>(rp.renderer.app.window.width()),
            -static_cast<float>(rp.renderer.app.window.height()),
            0.0f,
            1.0f};
    vkCmdSetViewport(rp.cb.get(), 0, 1, &viewport);

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

#include <planet/vk/engine2d/renderer.hpp>

#include <cstring>


using namespace std::literals;


planet::vk::engine2d::renderer::renderer(engine2d::app &a)
: app{a},
  screen_space{affine::transform2d{}
                       .translate({1.0f, -1.0f})
                       .scale(app.window.height() / 2, app.window.width() / 2)},
  viewport_buffer{
          buffer<coordinate_space>{
                  app.device.startup_memory, 1u,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
          buffer<coordinate_space>{
                  app.device.startup_memory, 1u,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
          buffer<coordinate_space>{
                  app.device.startup_memory, 1u,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}},
  viewport_mapping{
          viewport_buffer[0].map(), viewport_buffer[1].map(),
          viewport_buffer[2].map()} {

    for (auto &mapping : viewport_mapping) {
        std::memcpy(mapping.get(), &coordinates, sizeof(coordinate_space));
    }

    for (std::size_t index{}; auto const &vpb : viewport_buffer) {
        VkDescriptorBufferInfo info{};
        info.buffer = vpb.get();
        info.offset = 0;
        info.range = sizeof(coordinate_space);

        VkWriteDescriptorSet set{};
        set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        set.dstSet = ubo_sets[index];
        set.dstBinding = 0;
        set.dstArrayElement = 0;
        set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        set.descriptorCount = 1;
        set.pBufferInfo = &info;

        vkUpdateDescriptorSets(app.device.get(), 1, &set, 0, nullptr);

        ++index;
    }
}
planet::vk::engine2d::renderer::~renderer() {
    /// Because images can be in flight when we're destructed, we have to wait
    /// for them
    for (auto &f : fence) {
        std::array waitfor{f.get()};
        vkWaitForFences(
                app.device.get(), waitfor.size(), waitfor.data(), VK_TRUE,
                UINT64_MAX);
    }
}


planet::affine::matrix3d planet::vk::engine2d::renderer::correct_aspect_ratio(
        engine2d::app &app) {
    return affine::matrix3d::scale_xy(
            app.window.height() / app.window.width(), -1.0f);
}


void planet::vk::engine2d::renderer::reset_world_coordinates(
        affine::matrix3d const &m) {
    coordinates.world = m;
}


planet::vk::render_pass planet::vk::engine2d::renderer::create_render_pass() {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swap_chain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    planet::vk::render_pass render_pass{app.device, render_pass_info};
    swap_chain.create_frame_buffers(render_pass);
    return render_pass;
}


felspar::coro::task<std::size_t>
        planet::vk::engine2d::renderer::start(VkClearValue const colour) {
    // Wait for the previous version of this frame number to finish
    while (not fence[current_frame].is_ready()) {
        co_await app.sdl.io.sleep(5ms);
    }

    // Get an image from the swap chain
    image_index = 0;
    planet::vk::worked(vkAcquireNextImageKHR(
            app.device.get(), swap_chain.get(),
            std::numeric_limits<uint64_t>::max(),
            img_avail_semaphore[current_frame].get(), VK_NULL_HANDLE,
            &image_index));

    // We need to wait for the image before we can run the commands to draw
    // to it, and signal the render finished one when we're done
    fence[current_frame].reset();

    // Start to record command buffers
    auto &cb = command_buffers[current_frame];
    vkResetCommandBuffer(cb.get(), {});

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    planet::vk::worked(vkBeginCommandBuffer(cb.get(), &begin_info));

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass.get();
    render_pass_info.framebuffer =
            swap_chain.frame_buffers.at(image_index).get();
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent = swap_chain.extents;

    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &colour;

    vkCmdBeginRenderPass(
            cb.get(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    co_return current_frame;
}


void planet::vk::engine2d::renderer::submit_and_present() {
    auto &cb = command_buffers[current_frame];

    std::memcpy(
            viewport_mapping[current_frame].get(), &coordinates,
            sizeof(coordinate_space));

    vkCmdBindDescriptorSets(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            mesh.pipeline.layout.get(), 0, 1, &ubo_sets[current_frame], 0,
            nullptr);

    vkCmdBindPipeline(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, mesh.pipeline.get());
    mesh.render(*this, cb, current_frame);

    vkCmdBindPipeline(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, textured.pipeline.get());
    textured.render(*this, cb, current_frame);

    vkCmdBindPipeline(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, screen.pipeline.get());
    screen.render(*this, cb, current_frame);


    vkCmdEndRenderPass(cb.get());
    planet::vk::worked(vkEndCommandBuffer(cb.get()));

    std::array<VkSemaphore, 1> const wait_semaphores = {
            img_avail_semaphore[current_frame].get()};
    std::array<VkSemaphore, 1> const signal_semaphores = {
            render_finished_semaphore[current_frame].get()};
    std::array<VkPipelineStageFlags, 1> const wait_stages = {
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    std::array command_buffer{cb.get()};
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.commandBufferCount = command_buffer.size();
    submit_info.pCommandBuffers = command_buffer.data();
    submit_info.signalSemaphoreCount = signal_semaphores.size();
    submit_info.pSignalSemaphores = signal_semaphores.data();
    planet::vk::worked(vkQueueSubmit(
            app.device.graphics_queue, 1, &submit_info,
            fence[current_frame].get()));

    // Finally, present the updated image in the swap chain
    std::array<VkSwapchainKHR, 1> present_chain = {swap_chain.get()};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = signal_semaphores.size();
    present_info.pWaitSemaphores = signal_semaphores.data();
    present_info.swapchainCount = present_chain.size();
    present_info.pSwapchains = present_chain.data();
    present_info.pImageIndices = &image_index;
    planet::vk::worked(
            vkQueuePresentKHR(app.device.present_queue, &present_info));

    current_frame = (current_frame + 1) % max_frames_in_flight;
}


planet::vk::graphics_pipeline planet::vk::engine2d::create_graphics_pipeline(
        engine2d::app &app,
        std::string_view vert,
        std::string_view frag,
        std::span<VkVertexInputBindingDescription const> binding_description,
        std::span<VkVertexInputAttributeDescription const> attribute_description,
        view<vk::swap_chain> swap_chain,
        view<vk::render_pass> render_pass,
        vk::pipeline_layout pipeline_layout) {
    planet::vk::shader_module vertex_shader_module{
            app.device, app.asset_manager.file_data(vert)};
    planet::vk::shader_module fragment_shader_module{
            app.device, app.asset_manager.file_data(frag)};
    std::array shader_stages{
            vertex_shader_module.shader_stage_info(
                    VK_SHADER_STAGE_VERTEX_BIT, "main"),
            fragment_shader_module.shader_stage_info(
                    VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

    /// Vertex bindings and attributes
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount =
            binding_description.size();
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
            attribute_description.size();
    vertex_input_info.pVertexAttributeDescriptions =
            attribute_description.data();

    // Primitive type
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport config
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = app.window.zwidth();
    viewport.height = app.window.zheight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor rect config
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swap_chain().extents;

    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
    rasterizer_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blend_mode = {};
    blend_mode.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    blend_mode.blendEnable = VK_TRUE;
    blend_mode.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_mode.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_mode.colorBlendOp = VK_BLEND_OP_ADD;
    blend_mode.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_mode.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_mode.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.logicOpEnable = VK_FALSE;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_mode;

    VkGraphicsPipelineCreateInfo graphics_pipeline_info = {};
    graphics_pipeline_info.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.stageCount = 2;
    graphics_pipeline_info.pStages = shader_stages.data();
    graphics_pipeline_info.pVertexInputState = &vertex_input_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly;
    graphics_pipeline_info.pViewportState = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &rasterizer_info;
    graphics_pipeline_info.pMultisampleState = &multisampling;
    graphics_pipeline_info.pColorBlendState = &blend_info;
    graphics_pipeline_info.subpass = 0;

    return planet::vk::graphics_pipeline{
            app.device, graphics_pipeline_info, render_pass,
            std::move(pipeline_layout)};
}

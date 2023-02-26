#include <planet/vk/engine2d/renderer.hpp>

#include <cstring>


namespace {


    template<typename V>
    auto binding_description();

    template<typename V>
    auto attribute_description();

    template<>
    auto binding_description<planet::vk::engine2d::vertex>() {
        VkVertexInputBindingDescription description{};

        description.binding = 0;
        description.stride = sizeof(planet::vk::engine2d::vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::array{description};
    }

    template<>
    auto attribute_description<planet::vk::engine2d::vertex>() {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[0].offset = offsetof(planet::vk::engine2d::vertex, p);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[1].offset = offsetof(planet::vk::engine2d::vertex, c);

        return attrs;
    }


}


planet::vk::engine2d::renderer::renderer(engine2d::app &a)
: app{a},
  viewport{affine::matrix3d::scale_xy(
          1.0f, a.window.width() / a.window.height())},
  viewport_buffer{
          app.device, 1u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
  viewport_mapping{viewport_buffer.map()} {
    std::memcpy(viewport_mapping.get(), &viewport, sizeof(affine::matrix3d));

    VkDescriptorBufferInfo info{};
    info.buffer = viewport_buffer.get();
    info.offset = 0;
    info.range = sizeof(affine::matrix3d);

    VkWriteDescriptorSet set{};
    set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set.dstSet = ubo_sets[0];
    set.dstBinding = 0;
    set.dstArrayElement = 0;
    set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    set.descriptorCount = 1;
    set.pBufferInfo = &info;

    vkUpdateDescriptorSets(app.device.get(), 1, &set, 0, nullptr);
}


planet::vk::graphics_pipeline
        planet::vk::engine2d::renderer::create_mesh_pipeline() {
    planet::vk::shader_module vertex_shader_module{
            app.device,
            app.asset_manager.file_data(
                    "planet-vk-engine2d/2dmesh.vert.spirv")};
    planet::vk::shader_module fragment_shader_module{
            app.device,
            app.asset_manager.file_data(
                    "planet-vk-engine2d/2dmesh.frag.spirv")};
    std::array shader_stages{
            vertex_shader_module.shader_stage_info(
                    VK_SHADER_STAGE_VERTEX_BIT, "main"),
            fragment_shader_module.shader_stage_info(
                    VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

    /// Vertex bindings and attributes
    auto const binding = binding_description<vertex>();
    auto const attrs = attribute_description<vertex>();
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = binding.size();
    vertex_input_info.pVertexBindingDescriptions = binding.data();
    vertex_input_info.vertexAttributeDescriptionCount = attrs.size();
    vertex_input_info.pVertexAttributeDescriptions = attrs.data();

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
    scissor.extent = swapchain.extents;

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
    blend_mode.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.logicOpEnable = VK_FALSE;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_mode;

    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain.image_format;
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
    swapchain.create_frame_buffers(render_pass);

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
            app.device, graphics_pipeline_info, std::move(render_pass),
            planet::vk::pipeline_layout{app.device, ubo_layout}};
}


planet::vk::command_buffer &
        planet::vk::engine2d::renderer::start(VkClearValue const colour) {
    // Get an image from the swap chain
    image_index = 0;
    planet::vk::worked(vkAcquireNextImageKHR(
            app.device.get(), swapchain.get(),
            std::numeric_limits<uint64_t>::max(), img_avail_semaphore.get(),
            VK_NULL_HANDLE, &image_index));
    // We need to wait for the image before we can run the commands to draw
    // to it, and signal the render finished one when we're done
    planet::vk::worked(
            vkResetFences(app.device.get(), fences.size(), fences.data()));

    // Start to record command buffers
    auto &cb = command_buffers.buffers[image_index];

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    planet::vk::worked(vkBeginCommandBuffer(cb.get(), &begin_info));

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = mesh_pipeline.render_pass.get();
    render_pass_info.framebuffer = swapchain.frame_buffers[image_index].get();
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent = swapchain.extents;

    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &colour;

    vkCmdBeginRenderPass(
            cb.get(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    mesh2d_triangles.clear();
    mesh2d_indexes.clear();

    return cb;
}


void planet::vk::engine2d::renderer::draw_2dmesh(
        std::span<vertex const> const vertices,
        std::span<std::uint32_t const> const indices) {
    auto const start_index = mesh2d_triangles.size();
    for (auto const &v : vertices) { mesh2d_triangles.push_back(v); }
    for (auto const &i : indices) { mesh2d_indexes.push_back(start_index + i); }
}
void planet::vk::engine2d::renderer::draw_2dmesh(
        std::span<vertex const> const vertices,
        std::span<std::uint32_t const> const indices,
        pos const p) {
    auto const start_index = mesh2d_triangles.size();
    for (auto const &v : vertices) {
        mesh2d_triangles.push_back({v.p + p, v.c});
    }
    for (auto const &i : indices) { mesh2d_indexes.push_back(start_index + i); }
}


void planet::vk::engine2d::renderer::submit_and_present() {
    auto &cb = command_buffers.buffers[image_index];

    planet::vk::buffer<planet::vk::engine2d::vertex> vertex_buffer{
            app.device, mesh2d_triangles, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    planet::vk::buffer<std::uint32_t> index_buffer{
            app.device, mesh2d_indexes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    std::array buffers{vertex_buffer.get()};
    std::array offset{VkDeviceSize{}};

    vkCmdBindPipeline(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipeline.get());
    vkCmdBindVertexBuffers(
            cb.get(), 0, buffers.size(), buffers.data(), offset.data());
    vkCmdBindIndexBuffer(cb.get(), index_buffer.get(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            mesh_pipeline.layout.get(), 0, 1, &ubo_sets[0], 0, nullptr);
    vkCmdDrawIndexed(
            cb.get(), static_cast<uint32_t>(mesh2d_indexes.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(cb.get());
    planet::vk::worked(vkEndCommandBuffer(cb.get()));

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
            app.device.graphics_queue, 1, &submit_info, fence.get()));

    // Finally, present the updated image in the swap chain
    std::array<VkSwapchainKHR, 1> present_chain = {swapchain.get()};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = signal_semaphores.size();
    present_info.pWaitSemaphores = signal_semaphores.data();
    present_info.swapchainCount = present_chain.size();
    present_info.pSwapchains = present_chain.data();
    present_info.pImageIndices = &image_index;
    planet::vk::worked(
            vkQueuePresentKHR(app.device.present_queue, &present_info));

    // Wait for the frame to finish
    planet::vk::worked(vkWaitForFences(
            app.device.get(), fences.size(), fences.data(), true,
            std::numeric_limits<uint64_t>::max()));
}

#include <planet/telemetry/counter.hpp>
#include <planet/telemetry/rate.hpp>
#include <planet/vk/engine/renderer.hpp>

#include <algorithm>
#include <cstring>


using namespace std::literals;


/// ## `planet::vk::engine::renderer`


planet::vk::engine::renderer::renderer(engine::app &a)
: app{a},
  screen_space{
          affine::transform2d{}
                  .scale(2.0f / app.window.width(), -2.0f / app.window.height())
                  .translate({-1.0f, 1.0f})},
  logical_vulkan_space{affine::transform2d{}.scale(
          app.window.height() / app.window.width(), 1.0f)},
  coordinates{
          app.device.startup_memory,
          {.screen{screen_space.into()},
           .perspective{logical_vulkan_space.into()}}} {}
planet::vk::engine::renderer::~renderer() {
    /**
     * Because images can be in flight when we're destructed, we have to wait
     * for them
     */
    for (auto &f : fence) {
        std::array waitfor{f.get()};
        vkWaitForFences(
                app.device.get(), waitfor.size(), waitfor.data(), VK_TRUE,
                UINT64_MAX);
    }
}


void planet::vk::engine::renderer::reset_world_coordinates(
        affine::matrix3d const &m) {
    coordinates.current.world = m;
}
void planet::vk::engine::renderer::reset_world_coordinates(
        affine::matrix3d const &m, affine::matrix3d const &p) {
    coordinates.current.world = m;
    coordinates.current.perspective = p;
}


planet::vk::render_pass planet::vk::engine::renderer::create_render_pass() {
    auto attachments = std::array{
            colour_attachment.attachment_description(app.instance.gpu()),
            depth_buffer.attachment_description(app.instance.gpu()),
            swap_chain.attachment_description()};

    VkAttachmentReference colour_attachment_ref = {};
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colour_resolve_attachment_ref{};
    colour_resolve_attachment_ref.attachment = 2;
    colour_resolve_attachment_ref.layout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colour_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    subpass.pResolveAttachments = &colour_resolve_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = attachments.size();
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    planet::vk::render_pass render_pass{app.device, render_pass_info};
    swap_chain.create_frame_buffers(
            render_pass, colour_attachment.image_view.get(),
            depth_buffer.image_view.get());
    return render_pass;
}


namespace {
    planet::telemetry::counter c_recreate_swapchain{
            "planet_vk_engine_renderer_recreate_swapchain_count"};
}
void planet::vk::engine::renderer::recreate_swap_chain(
        VkResult const result, felspar::source_location const &loc) {
    ++c_recreate_swapchain;
    auto const images =
            swap_chain.recreate(app.window.refresh_window_dimensions());
    swap_chain.create_frame_buffers(
            render_pass, colour_attachment.image_view.get(),
            depth_buffer.image_view.get());
    planet::log::info(
            "Swap chain dirty. New image count", images, detail::error(result),
            loc);
}


namespace {
    planet::telemetry::real_time_rate c_fence_wait{
            "planet_vk_engine_renderer_fence_wait", 500ms};
    planet::telemetry::real_time_rate c_acquire_wait{
            "planet_vk_engine_renderer_acquire_next_image_wait", 500ms};
}
felspar::coro::task<std::size_t>
        planet::vk::engine::renderer::start(VkClearValue const colour) {
    constexpr auto wait_time = 5ms;
    // Wait for the previous version of this frame number to finish
    while (not fence[current_frame].is_ready()) {
        c_fence_wait.tick();
        co_await app.sdl.io.sleep(wait_time);
    }

    // Get an image from the swap chain
    image_index = 0;
    while (true) {
        auto result = vkAcquireNextImageKHR(
                app.device.get(), swap_chain.get(), {},
                img_avail_semaphore[current_frame].get(), VK_NULL_HANDLE,
                &image_index);
        if (result == VK_TIMEOUT or result == VK_NOT_READY) {
            c_acquire_wait.tick();
            co_await app.sdl.io.sleep(wait_time);
        } else if (
                result == VK_ERROR_OUT_OF_DATE_KHR
                or result == VK_SUBOPTIMAL_KHR) {
            recreate_swap_chain(result);
        } else if (result == VK_SUCCESS) {
            break;
        } else {
            planet::vk::worked(result);
        }
    }

    // We need to wait for the image before we can run the commands to draw
    // to it, and signal the render finished one when we're done
    fence[current_frame].reset();

    // Resume any processing waiting for the frames to cycle around
    for (auto h : render_cycle_coroutines.front()) { h.resume(); }
    render_cycle_coroutines.front().clear();
    std::rotate(
            render_cycle_coroutines.begin(),
            render_cycle_coroutines.begin() + 1, render_cycle_coroutines.end());
    for (auto h : pre_start_coroutines) { h.resume(); }
    pre_start_coroutines.clear();

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

    std::array<VkClearValue, 2> clear_values{
            colour, {.depthStencil = {0.0f, 0}}};
    render_pass_info.clearValueCount = clear_values.size();
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(
            cb.get(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = app.window.height();
    viewport.width = app.window.width();
    viewport.height = -app.window.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cb.get(), 0, 1, &viewport);

    coordinates.copy_to_gpu_memory(current_frame);

    app.baseplate.start_frame_reset();

    co_return current_frame;
}


auto planet::vk::engine::renderer::bind(planet::vk::graphics_pipeline &pl)
        -> planet::vk::engine::render_parameters {
    auto &cb = command_buffers[current_frame];
    vkCmdBindPipeline(cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, pl.get());
    vkCmdBindDescriptorSets(
            cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS, pl.layout.get(), 0, 1,
            &coordinates.vk.sets[current_frame], 0, nullptr);
    return {*this, cb, current_frame};
}


namespace {
    planet::telemetry::counter frame_count{
            "planet_vk_engine_renderer_frame_count"};
    planet::telemetry::real_time_rate frame_rate{
            "planet_vk_engine_renderer_frame_rate", 500ms};
}
void planet::vk::engine::renderer::submit_and_present() {
    auto &cb = command_buffers[current_frame];

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
    auto const presented =
            vkQueuePresentKHR(app.device.present_queue, &present_info);
    if (presented == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain(presented);
    } else {
        worked(presented);
    }

    current_frame = (current_frame + 1) % max_frames_in_flight;
    ++frame_count;
    frame_rate.tick();
}


/// ## `planet::vk::engine::renderer::render_cycle_awaitable`


planet::vk::engine::renderer::render_cycle_awaitable::~render_cycle_awaitable() {
    if (mine) {
        for (auto &frame : renderer.render_cycle_coroutines) {
            std::erase(frame, mine);
        }
    }
}


void planet::vk::engine::renderer::render_cycle_awaitable::await_suspend(
        felspar::coro::coroutine_handle<> h) {
    mine = h;
    renderer.render_cycle_coroutines.back().push_back(h);
}


/// ## `planet::vk::engine::renderer::render_prestart_awaitable`


planet::vk::engine::renderer::render_prestart_awaitable::
        ~render_prestart_awaitable() {
    if (mine) { std::erase(renderer.pre_start_coroutines, mine); }
}


void planet::vk::engine::renderer::render_prestart_awaitable::await_suspend(
        felspar::coro::coroutine_handle<> h) {
    mine = h;
    renderer.pre_start_coroutines.push_back(h);
}


std::size_t
        planet::vk::engine::renderer::render_prestart_awaitable::await_resume()
                const noexcept {
    return renderer.current_frame;
}


/// ## `planet::vk::engine`


planet::vk::graphics_pipeline planet::vk::engine::create_graphics_pipeline(
        graphics_pipeline_parameters parameters) {
    auto &app = parameters.app;

    /// Shaders
    planet::vk::shader_module vertex_shader_module{
            app.device, app.asset_manager.file_data(parameters.vertex_shader)};
    planet::vk::shader_module fragment_shader_module{
            app.device,
            app.asset_manager.file_data(parameters.fragment_shader)};
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
            parameters.binding_descriptions.size();
    vertex_input_info.pVertexBindingDescriptions =
            parameters.binding_descriptions.data();
    vertex_input_info.vertexAttributeDescriptionCount =
            parameters.attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions =
            parameters.attribute_descriptions.data();

    // Primitive type
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport config
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = app.window.height();
    viewport.width = app.window.width();
    viewport.height = -app.window.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    planet::log::debug(
            "Viewport", viewport.x, viewport.y, viewport.width,
            viewport.height);

    // Scissor rect config
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = parameters.extents;

    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    // Set up dynamic state
    static auto constexpr dynamic_states =
            std::array{VK_DYNAMIC_STATE_VIEWPORT};
    VkPipelineDynamicStateCreateInfo pipleline_dynamic_states{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .dynamicStateCount = dynamic_states.size(),
            .pDynamicStates = dynamic_states.data()};

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
    multisampling.rasterizationSamples = app.instance.gpu().msaa_samples;

    VkPipelineColorBlendAttachmentState blend_state = {};
    blend_state.blendEnable = VK_TRUE;
    blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_state.colorBlendOp = VK_BLEND_OP_ADD;
    blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
    switch (parameters.blend_mode) {
    case blend_mode::none: blend_state.blendEnable = VK_FALSE; break;
    case blend_mode::multiply:
        blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case blend_mode::add:
        blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        break;
    }

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.logicOpEnable = VK_FALSE;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_state;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo graphics_pipeline_info = {};
    graphics_pipeline_info.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.stageCount = shader_stages.size();
    graphics_pipeline_info.pStages = shader_stages.data();
    graphics_pipeline_info.pVertexInputState = &vertex_input_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly;
    graphics_pipeline_info.pViewportState = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &rasterizer_info;
    graphics_pipeline_info.pMultisampleState = &multisampling;
    graphics_pipeline_info.pDepthStencilState = &depth_stencil;
    graphics_pipeline_info.pColorBlendState = &blend_info;
    graphics_pipeline_info.pDynamicState = &pipleline_dynamic_states;
    graphics_pipeline_info.subpass = parameters.sub_pass;

    return planet::vk::graphics_pipeline{
            app.device, graphics_pipeline_info, parameters.render_pass,
            std::move(parameters.pipeline_layout)};
}

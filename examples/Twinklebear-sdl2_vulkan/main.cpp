#include <planet/vk-sdl.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <SDL.h>
#include <SDL_vulkan.h>

constexpr int win_width = 1280;
constexpr int win_height = 720;


int main(int, const char **argv) {
    planet::version const v{"sdl2_vulkan", "Twinklebear/sdl2_vulkan", "0.1", 1};
    planet::asset_manager assets{argv[0]};
    felspar::io::poll_warden poll;
    planet::sdl::init sdl{poll, v};

    planet::vk::sdl::window window{sdl, "SDL2 + Vulkan", win_width, win_height};

    planet::vk::extensions extensions{window};
#ifdef NDEBUG
    extensions.validation_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    // Make the Vulkan Instance
    planet::vk::instance vk_instance = [&]() {
        auto app_info = planet::vk::application_info();
        app_info.pApplicationName = "SDL2 + Vulkan";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        auto info = planet::vk::instance::info(extensions, app_info);
        return planet::vk::instance{
                extensions, info, [&](VkInstance h) {
                    std::cout << "Requested extensions\n";
                    for (auto ex : extensions.vulkan_extensions) {
                        std::cout << ex << '\n';
                    }
                    std::cout << "Requested validation layers\n";
                    for (auto ex : extensions.validation_layers) {
                        std::cout << ex << '\n';
                    }

                    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
                    if (not SDL_Vulkan_CreateSurface(
                                window.get(), h, &vk_surface)) {
                        throw felspar::stdexcept::runtime_error{
                                "SDL_Vulkan_CreateSurface failed"};
                    }
                    return vk_surface;
                }};
    }();

    std::cout << "Found " << vk_instance.physical_devices().size()
              << " devices. Using " << vk_instance.gpu().properties.deviceName
              << "\n";
    std::cout << "Graphics queue family is "
              << vk_instance.surface.graphics_queue_family_index()
              << " and presentation queue family is "
              << vk_instance.surface.presentation_queue_family_index() << "\n";

    planet::vk::device vk_device{vk_instance, extensions};
    planet::vk::swap_chain vk_swapchain{
            vk_device, VkExtent2D{win_width, win_height}};

    // Build the pipeline
    planet::vk::render_pass vk_render_pass{[&]() {
        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        VkAttachmentDescription color_attachment = {};
        color_attachment.format = vk_swapchain.image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        return planet::vk::render_pass{vk_device, render_pass_info};
    }()};
    planet::vk::graphics_pipeline vk_pipeline{[&]() {
        planet::vk::shader_module vertex_shader_module{
                vk_device, assets.file_data("vertex.vert.spirv")};
        planet::vk::shader_module fragment_shader_module{
                vk_device, assets.file_data("fragment.frag.spirv")};
        std::array shader_stages{
                vertex_shader_module.shader_stage_info(
                        VK_SHADER_STAGE_VERTEX_BIT, "main"),
                fragment_shader_module.shader_stage_info(
                        VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

        // Vertex data hard-coded in vertex shader
        VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
        vertex_input_info.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = 0;
        vertex_input_info.vertexAttributeDescriptionCount = 0;

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
        viewport.width = win_width;
        viewport.height = win_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor rect config
        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = vk_swapchain.extents;

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
        blend_info.sType =
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
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
                vk_device, graphics_pipeline_info, vk_render_pass,
                planet::vk::pipeline_layout{vk_device}};
    }()};

    // Setup framebuffers
    vk_swapchain.create_frame_buffers(vk_pipeline.render_pass);

    // Setup the command pool
    planet::vk::command_pool vk_command_pool{vk_device, vk_instance.surface};
    planet::vk::command_buffers command_buffers{
            vk_command_pool, vk_swapchain.frame_buffers.size()};

    // Now record the rendering commands (TODO: Could also do this pre-recording
    // in the DXR backend of rtobj. Will there be much perf. difference?)
    for (size_t i = 0; i < command_buffers.size(); ++i) {
        auto &cmd_buf = command_buffers[i];

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        planet::vk::worked(vkBeginCommandBuffer(cmd_buf.get(), &begin_info));

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = vk_pipeline.render_pass.get();
        render_pass_info.framebuffer = vk_swapchain.frame_buffers[i].get();
        render_pass_info.renderArea.offset.x = 0;
        render_pass_info.renderArea.offset.y = 0;
        render_pass_info.renderArea.extent = vk_swapchain.extents;

        VkClearValue clear_color = {{{0.f, 0.f, 0.f, 1.f}}};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(
                cmd_buf.get(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
                cmd_buf.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                vk_pipeline.get());

        // Draw our "triangle" embedded in the shader
        vkCmdDraw(cmd_buf.get(), 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd_buf.get());

        planet::vk::worked(vkEndCommandBuffer(cmd_buf.get()));
    }

    planet::vk::semaphore img_avail_semaphore{vk_device},
            render_finished_semaphore{vk_device};
    // We use a fence to wait for the rendering work to finish
    planet::vk::fence vk_fence{vk_device};

    std::cout << "Running loop\n";
    bool done = false;
    while (not done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { done = true; }
            if (event.type == SDL_KEYDOWN
                && event.key.keysym.sym == SDLK_ESCAPE) {
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_CLOSE
                && event.window.windowID == SDL_GetWindowID(window.get())) {
                done = true;
            }
        }

        // Get an image from the swap chain
        uint32_t img_index = 0;
        planet::vk::worked(vkAcquireNextImageKHR(
                vk_device.get(), vk_swapchain.get(),
                std::numeric_limits<uint64_t>::max(), img_avail_semaphore.get(),
                VK_NULL_HANDLE, &img_index));

        // We need to wait for the image before we can run the commands to draw
        // to it, and signal the render finished one when we're done
        std::array<VkSemaphore, 1> const wait_semaphores = {
                img_avail_semaphore.get()};
        std::array<VkSemaphore, 1> const signal_semaphores = {
                render_finished_semaphore.get()};
        std::array<VkPipelineStageFlags, 1> const wait_stages = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        std::array const fences{vk_fence.get()};

        planet::vk::worked(
                vkResetFences(vk_device.get(), fences.size(), fences.data()));

        std::array command_buffer{command_buffers[img_index].get()};
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
                vk_device.graphics_queue, 1, &submit_info, vk_fence.get()));

        // Finally, present the updated image in the swap chain
        std::array<VkSwapchainKHR, 1> present_chain = {vk_swapchain.get()};
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = signal_semaphores.size();
        present_info.pWaitSemaphores = signal_semaphores.data();
        present_info.swapchainCount = present_chain.size();
        present_info.pSwapchains = present_chain.data();
        present_info.pImageIndices = &img_index;
        planet::vk::worked(
                vkQueuePresentKHR(vk_device.present_queue, &present_info));

        // Wait for the frame to finish
        planet::vk::worked(vkWaitForFences(
                vk_device.get(), fences.size(), fences.data(), true,
                std::numeric_limits<uint64_t>::max()));
    }
    return 0;
}

#include <planet/vk-sdl.hpp>

#include <felspar/memory/small_vector.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <SDL.h>
#include <SDL_vulkan.h>

constexpr int win_width = 1280;
constexpr int win_height = 720;


int main(int argc, const char **argv) {
    planet::asset_manager assets{argv[0]};
    felspar::io::poll_warden poll;
    planet::sdl::init sdl{poll};

    planet::vk::sdl::window window{sdl, "SDL2 + Vulkan", win_width, win_height};

    {
        auto extensions = planet::vk::fetch_vector<
                vkEnumerateInstanceExtensionProperties, VkExtensionProperties>(
                nullptr);
        std::cout << "num extensions: " << extensions.size() << "\n";
        std::cout << "Available extensions:\n";
        for (const auto &e : extensions) {
            std::cout << e.extensionName << "\n";
        }
    }

    planet::vk::extensions extensions{window};
    extensions.validation_layers.push_back("VK_LAYER_KHRONOS_validation");
    std::cout << "Requested extensions\n";
    for (auto ex : extensions.vulkan_extensions) { std::cout << ex << '\n'; }

    // Make the Vulkan Instance
    planet::vk::instance vk_instance;
    {
        auto app_info = planet::vk::application_info();
        app_info.pApplicationName = "SDL2 + Vulkan";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

        vk_instance = planet::vk::instance{
                planet::vk::instance::info(extensions, app_info),
                [&window](VkInstance h) {
                    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
                    if (not SDL_Vulkan_CreateSurface(
                                window.get(), h, &vk_surface)) {
                        throw felspar::stdexcept::runtime_error{
                                "SDL_Vulkan_CreateSurface failed"};
                    }
                    return vk_surface;
                }};
    }

    std::cout << "Found " << vk_instance.devices().size() << " devices\n";
    std::cout << "Using " << vk_instance.gpu().properties.deviceName << "\n";

    VkDevice vk_device = VK_NULL_HANDLE;
    VkQueue vk_graphics_queue = VK_NULL_HANDLE,
            vk_present_queue = VK_NULL_HANDLE;
    {
        std::cout << "Graphics queue is "
                  << vk_instance.gpu().graphics_queue_index()
                  << " and presentation queue is "
                  << vk_instance.gpu().presentation_queue_index() << "\n";

        felspar::memory::small_vector<VkDeviceQueueCreateInfo, 2>
                queue_create_infos;
        const float queue_priority = 1.f;
        for (auto const q : std::array{
                     vk_instance.gpu().graphics_queue_index(),
                     vk_instance.gpu().presentation_queue_index()}) {
            queue_create_infos.emplace_back();
            queue_create_infos.back().sType =
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos.back().queueFamilyIndex = q;
            queue_create_infos.back().queueCount = 1;
            queue_create_infos.back().pQueuePriorities = &queue_priority;
        }

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.enabledLayerCount = extensions.validation_layers.size();
        create_info.ppEnabledLayerNames = extensions.validation_layers.data();
        create_info.enabledExtensionCount = extensions.device_extensions.size();
        create_info.ppEnabledExtensionNames =
                extensions.device_extensions.data();
        create_info.pEnabledFeatures = &device_features;
        planet::vk::worked(vkCreateDevice(
                vk_instance.gpu().get(), &create_info, nullptr, &vk_device));

        vkGetDeviceQueue(
                vk_device, vk_instance.gpu().graphics_queue_index(), 0,
                &vk_graphics_queue);
        vkGetDeviceQueue(
                vk_device, vk_instance.gpu().presentation_queue_index(), 0,
                &vk_present_queue);
    }

    // Setup swapchain, assume a real GPU so don't bother querying the
    // capabilities, just get what we want
    VkExtent2D swapchain_extent = {};
    swapchain_extent.width = win_width;
    swapchain_extent.height = win_height;
    const VkFormat swapchain_img_format = VK_FORMAT_B8G8R8A8_UNORM;

    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    {
        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = vk_instance.surface;
        create_info.minImageCount = 2;
        create_info.imageFormat = swapchain_img_format;
        create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        create_info.imageExtent = swapchain_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // We only have 1 queue
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        create_info.clipped = true;
        create_info.oldSwapchain = VK_NULL_HANDLE;
        planet::vk::worked(vkCreateSwapchainKHR(
                vk_device, &create_info, nullptr, &vk_swapchain));

        // Get the swap chain images
        uint32_t num_swapchain_imgs = 0;
        vkGetSwapchainImagesKHR(
                vk_device, vk_swapchain, &num_swapchain_imgs, nullptr);
        swapchain_images.resize(num_swapchain_imgs);
        vkGetSwapchainImagesKHR(
                vk_device, vk_swapchain, &num_swapchain_imgs,
                swapchain_images.data());

        for (const auto &img : swapchain_images) {
            VkImageViewCreateInfo view_create_info = {};
            view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_create_info.image = img;
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_create_info.format = swapchain_img_format;

            view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            view_create_info.subresourceRange.aspectMask =
                    VK_IMAGE_ASPECT_COLOR_BIT;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            VkImageView img_view;
            planet::vk::worked(vkCreateImageView(
                    vk_device, &view_create_info, nullptr, &img_view));
            swapchain_image_views.push_back(img_view);
        }
    }

    // Build the pipeline
    VkPipelineLayout vk_pipeline_layout;
    VkRenderPass vk_render_pass;
    VkPipeline vk_pipeline;
    {
        auto vert_spv = assets.file_data("vert.vert.spirv");
        auto frag_spv = assets.file_data("frag.frag.spirv");
        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;

        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = vert_spv.size();
        create_info.pCode = reinterpret_cast<std::uint32_t *>(vert_spv.data());
        planet::vk::worked(vkCreateShaderModule(
                vk_device, &create_info, nullptr, &vertex_shader_module));

        VkPipelineShaderStageCreateInfo vertex_stage = {};
        vertex_stage.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = vertex_shader_module;
        vertex_stage.pName = "main";

        VkShaderModule fragment_shader_module = VK_NULL_HANDLE;
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = frag_spv.size();
        create_info.pCode = reinterpret_cast<std::uint32_t *>(frag_spv.data());
        planet::vk::worked(vkCreateShaderModule(
                vk_device, &create_info, nullptr, &fragment_shader_module));

        VkPipelineShaderStageCreateInfo fragment_stage = {};
        fragment_stage.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_stage.module = fragment_shader_module;
        fragment_stage.pName = "main";

        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
                vertex_stage, fragment_stage};

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
        scissor.extent = swapchain_extent;

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

        VkPipelineLayoutCreateInfo pipeline_info = {};
        pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        planet::vk::worked(vkCreatePipelineLayout(
                vk_device, &pipeline_info, nullptr, &vk_pipeline_layout));

        VkAttachmentDescription color_attachment = {};
        color_attachment.format = swapchain_img_format;
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
        planet::vk::worked(vkCreateRenderPass(
                vk_device, &render_pass_info, nullptr, &vk_render_pass));

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
        graphics_pipeline_info.layout = vk_pipeline_layout;
        graphics_pipeline_info.renderPass = vk_render_pass;
        graphics_pipeline_info.subpass = 0;
        planet::vk::worked(vkCreateGraphicsPipelines(
                vk_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr,
                &vk_pipeline));

        vkDestroyShaderModule(vk_device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(vk_device, fragment_shader_module, nullptr);
    }

    // Setup framebuffers
    std::vector<VkFramebuffer> framebuffers;
    for (const auto &v : swapchain_image_views) {
        std::array<VkImageView, 1> attachments = {v};
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = vk_render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments.data();
        create_info.width = win_width;
        create_info.height = win_height;
        create_info.layers = 1;
        VkFramebuffer fb = VK_NULL_HANDLE;
        planet::vk::worked(
                vkCreateFramebuffer(vk_device, &create_info, nullptr, &fb));
        framebuffers.push_back(fb);
    }

    // Setup the command pool
    VkCommandPool vk_command_pool;
    {
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = vk_instance.gpu().graphics_queue_index();
        planet::vk::worked(vkCreateCommandPool(
                vk_device, &create_info, nullptr, &vk_command_pool));
    }

    std::vector<VkCommandBuffer> command_buffers(
            framebuffers.size(), VkCommandBuffer{});
    {
        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = vk_command_pool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = command_buffers.size();
        planet::vk::worked(vkAllocateCommandBuffers(
                vk_device, &info, command_buffers.data()));
    }

    // Now record the rendering commands (TODO: Could also do this pre-recording
    // in the DXR backend of rtobj. Will there be much perf. difference?)
    for (size_t i = 0; i < command_buffers.size(); ++i) {
        auto &cmd_buf = command_buffers[i];

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        planet::vk::worked(vkBeginCommandBuffer(cmd_buf, &begin_info));

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = vk_render_pass;
        render_pass_info.framebuffer = framebuffers[i];
        render_pass_info.renderArea.offset.x = 0;
        render_pass_info.renderArea.offset.y = 0;
        render_pass_info.renderArea.extent = swapchain_extent;

        VkClearValue clear_color = {0.f, 0.f, 0.f, 1.f};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(
                cmd_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
                cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        // Draw our "triangle" embedded in the shader
        vkCmdDraw(cmd_buf, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd_buf);

        planet::vk::worked(vkEndCommandBuffer(cmd_buf));
    }

    VkSemaphore img_avail_semaphore = VK_NULL_HANDLE;
    VkSemaphore render_finished_semaphore = VK_NULL_HANDLE;
    {
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        planet::vk::worked(vkCreateSemaphore(
                vk_device, &info, nullptr, &img_avail_semaphore));
        planet::vk::worked(vkCreateSemaphore(
                vk_device, &info, nullptr, &render_finished_semaphore));
    }

    // We use a fence to wait for the rendering work to finish
    VkFence vk_fence = VK_NULL_HANDLE;
    {
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        planet::vk::worked(vkCreateFence(vk_device, &info, nullptr, &vk_fence));
    }

    std::cout << "Running loop\n";
    bool done = false;
    while (!done) {
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
                vk_device, vk_swapchain, std::numeric_limits<uint64_t>::max(),
                img_avail_semaphore, VK_NULL_HANDLE, &img_index));

        // We need to wait for the image before we can run the commands to draw
        // to it, and signal the render finished one when we're done
        const std::array<VkSemaphore, 1> wait_semaphores = {
                img_avail_semaphore};
        const std::array<VkSemaphore, 1> signal_semaphores = {
                render_finished_semaphore};
        const std::array<VkPipelineStageFlags, 1> wait_stages = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

        planet::vk::worked(vkResetFences(vk_device, 1, &vk_fence));

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers[img_index];
        submit_info.signalSemaphoreCount = signal_semaphores.size();
        submit_info.pSignalSemaphores = signal_semaphores.data();
        planet::vk::worked(
                vkQueueSubmit(vk_graphics_queue, 1, &submit_info, vk_fence));

        // Finally, present the updated image in the swap chain
        std::array<VkSwapchainKHR, 1> present_chain = {vk_swapchain};
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = signal_semaphores.size();
        present_info.pWaitSemaphores = signal_semaphores.data();
        present_info.swapchainCount = present_chain.size();
        present_info.pSwapchains = present_chain.data();
        present_info.pImageIndices = &img_index;
        planet::vk::worked(vkQueuePresentKHR(vk_present_queue, &present_info));

        // Wait for the frame to finish
        planet::vk::worked(vkWaitForFences(
                vk_device, 1, &vk_fence, true,
                std::numeric_limits<uint64_t>::max()));
    }

    vkDestroySemaphore(vk_device, img_avail_semaphore, nullptr);
    vkDestroySemaphore(vk_device, render_finished_semaphore, nullptr);
    vkDestroyFence(vk_device, vk_fence, nullptr);
    vkDestroyCommandPool(vk_device, vk_command_pool, nullptr);
    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
    for (auto &fb : framebuffers) {
        vkDestroyFramebuffer(vk_device, fb, nullptr);
    }
    vkDestroyPipeline(vk_device, vk_pipeline, nullptr);
    vkDestroyRenderPass(vk_device, vk_render_pass, nullptr);
    vkDestroyPipelineLayout(vk_device, vk_pipeline_layout, nullptr);
    for (auto &v : swapchain_image_views) {
        vkDestroyImageView(vk_device, v, nullptr);
    }
    vkDestroyDevice(vk_device, nullptr);

    return 0;
}

#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d {


    struct pos {
        float x, y;
    };
    struct colour {
        float r, g, b;
    };
    struct vertex {
        pos p;
        colour c;
    };


    class renderer final {
        vk::graphics_pipeline create_pipeline();

      public:
        renderer(engine2d::app &);

        engine2d::app &app;

        vk::swap_chain swapchain{app.device, app.window.extents()};
        vk::graphics_pipeline pipeline{create_pipeline()};

        vk::command_pool command_pool{app.device, app.instance.surface};
        vk::command_buffers command_buffers{
                command_pool, swapchain.frame_buffers.size()};

        planet::vk::semaphore img_avail_semaphore{app.device},
                render_finished_semaphore{app.device};
        planet::vk::fence fence{app.device};

        /// ## Drawing API

        /// Data we need to track whilst in the render loop
        std::uint32_t image_index = {};
        std::array<VkSemaphore, 1> const wait_semaphores = {
                img_avail_semaphore.get()};
        std::array<VkSemaphore, 1> const signal_semaphores = {
                render_finished_semaphore.get()};
        std::array<VkPipelineStageFlags, 1> const wait_stages = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        std::array<VkFence, 1> const fences{fence.get()};

        /// Start the render cycle
        [[nodiscard]] vk::command_buffer &start(VkClearValue);

        /// Submit and present the frame. This blocks until the frame is complete
        void submit_and_present();
    };


}

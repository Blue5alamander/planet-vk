#pragma once


#include <planet/vk/engine2d/app.hpp>

#include <planet/affine/matrix3d.hpp>


namespace planet::vk::engine2d {


    struct pos {
        float x, y;

        friend constexpr pos operator+(pos const l, pos const r) {
            return {l.x + r.x, l.y + r.y};
        }
    };
    struct colour {
        float r, g, b;
    };
    struct vertex {
        pos p;
        colour c;
    };


    /// ## Renderer
    class renderer final {
        vk::graphics_pipeline create_mesh_pipeline();
        std::vector<vertex> mesh2d_triangles;
        std::vector<std::uint32_t> mesh2d_indexes;

      public:
        renderer(engine2d::app &);

        engine2d::app &app;

        vk::swap_chain swapchain{app.device, app.window.extents()};
        vk::graphics_pipeline mesh_pipeline{create_mesh_pipeline()};

        vk::command_pool command_pool{app.device, app.instance.surface};
        vk::command_buffers command_buffers{
                command_pool, swapchain.frame_buffers.size()};

        planet::vk::semaphore img_avail_semaphore{app.device},
                render_finished_semaphore{app.device};
        planet::vk::fence fence{app.device};

        /// ### Drawing API

        affine::matrix3d viewport;

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
        vk::command_buffer &start(VkClearValue);

        /// Draw a 2D triangle mesh with an optional positional offset
        void draw_2dmesh(
                std::span<vertex const>, std::span<std::uint32_t const>);
        void draw_2dmesh(
                std::span<vertex const>, std::span<std::uint32_t const>, pos);

        /// Submit and present the frame. This blocks until the frame is complete
        void submit_and_present();
    };


}

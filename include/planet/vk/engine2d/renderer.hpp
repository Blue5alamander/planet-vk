#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/vk/engine2d/app.hpp>

#include <planet/affine/matrix3d.hpp>


namespace planet::vk::engine2d {


    struct pos {
        float x, y;

        friend constexpr pos operator+(pos const l, pos const r) {
            return {l.x + r.x, l.y + r.y};
        }
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
        vk::descriptor_set_layout ubo_layout{
                vk::descriptor_set_layout::for_uniform_buffer_object(
                        app.device)};
        vk::graphics_pipeline mesh_pipeline{create_mesh_pipeline()};

        vk::command_pool command_pool{app.device, app.instance.surface};
        vk::command_buffers command_buffers{
                command_pool, swapchain.frame_buffers.size()};

        vk::semaphore img_avail_semaphore{app.device},
                render_finished_semaphore{app.device};
        vk::fence fence{app.device};

        /// ### Drawing API

        /// #### Data we need to track whilst in the render loop
        std::uint32_t image_index = {};
        std::array<VkSemaphore, 1> const wait_semaphores = {
                img_avail_semaphore.get()};
        std::array<VkSemaphore, 1> const signal_semaphores = {
                render_finished_semaphore.get()};
        std::array<VkPipelineStageFlags, 1> const wait_stages = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        std::array<VkFence, 1> const fences{fence.get()};

        /// #### Start the render cycle
        vk::command_buffer &start(VkClearValue);

        /// #### Draw a 2D triangle mesh with an optional positional offset
        void draw_2dmesh(
                std::span<vertex const>, std::span<std::uint32_t const>);
        void draw_2dmesh(
                std::span<vertex const>, std::span<std::uint32_t const>, pos);
        void draw_2dmesh(
                std::span<vertex const>,
                std::span<std::uint32_t const>,
                pos,
                colour const &);

        /// #### Submit and present the frame
        /// This blocks until the frame is complete
        void submit_and_present();

        /// ### View space mapping

        /// #### Calculate square aspect ratio
        /**
         * The Vulkan coordinate system maps both width and height to -1 to +1.
         * This corrects the width and height so that the narrowest dimension is
         * in the range -1 to +1 and the widest is adjusted to give a square
         * aspect.
         */
        static affine::matrix3d correct_aspect_ratio(engine2d::app &);

        /// #### Reset the view matrix
        void reset_viewport(affine::matrix3d const &);

      private:
        affine::matrix3d viewport{correct_aspect_ratio(app)};
        buffer<affine::matrix3d> viewport_buffer;
        device_memory::mapping viewport_mapping;

        vk::descriptor_pool ubo_pool{app.device};
        vk::descriptor_sets ubo_sets{ubo_pool, ubo_layout};
    };


}

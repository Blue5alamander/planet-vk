#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/vk/engine2d/app.hpp>
#include <planet/vk/engine2d/mesh.pipeline.hpp>
#include <planet/vk/engine2d/textured.pipeline.hpp>

#include <planet/affine/matrix3d.hpp>


namespace planet::vk::engine2d {


    /// ## Renderer
    class renderer final {
        std::size_t current_frame = {};
        vk::render_pass create_render_pass();

      public:
        renderer(engine2d::app &);
        ~renderer();

        engine2d::app &app;


        /// ### Per-frame memory allocator
        device_memory_allocator per_frame_memory{app.device};


        /// ### Swap chain, command buffers and synchronisation
        vk::swap_chain swap_chain{app.device, app.window.extents()};
        vk::descriptor_set_layout ubo_layout{
                vk::descriptor_set_layout::for_uniform_buffer_object(
                        app.device)};
        vk::render_pass render_pass{create_render_pass()};

        vk::command_pool command_pool{app.device, app.instance.surface};
        vk::command_buffers command_buffers{command_pool, max_frames_in_flight};

        std::array<vk::semaphore, max_frames_in_flight> img_avail_semaphore{
                app.device, app.device, app.device},
                render_finished_semaphore{app.device, app.device, app.device};
        std::array<vk::fence, max_frames_in_flight> fence{
                app.device, app.device, app.device};


        /// ### Pipelines

        pipeline::mesh mesh{app, swap_chain, render_pass, ubo_layout};
        pipeline::textured textured{app, swap_chain, render_pass, ubo_layout};


        /// ### Drawing API

        /// #### Start the render cycle
        /**
         * Returns the current frame index. This is the index between zero and
         * `max_frames_in_flight` that is currently being worked on.
         */
        felspar::coro::task<std::size_t> start(VkClearValue);

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
        /// ### Data we need to track whilst in the render loop

        std::uint32_t image_index = {};

        /// ### View port transformation matrix and UBO

        affine::matrix3d viewport{correct_aspect_ratio(app)};
        std::array<buffer<affine::matrix3d>, max_frames_in_flight>
                viewport_buffer;
        std::array<device_memory::mapping, max_frames_in_flight> viewport_mapping;

        vk::descriptor_pool ubo_pool{app.device, max_frames_in_flight};
        vk::descriptor_sets ubo_sets{
                ubo_pool, ubo_layout, max_frames_in_flight};
    };


}

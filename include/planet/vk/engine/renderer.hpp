#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/depth_buffer.hpp>
#include <planet/vk/engine/forward.hpp>
#include <planet/vk/engine/render_parameters.hpp>

#include <planet/affine/matrix3d.hpp>


namespace planet::vk::engine {


    /// ## Renderer
    class renderer final {
        template<typename T>
        friend struct ui::per_frame;

        std::size_t current_frame = {};
        vk::render_pass create_render_pass();

      public:
        renderer(engine::app &);
        ~renderer();

        engine::app &app;


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

        engine::depth_buffer depth_buffer;

        std::array<vk::semaphore, max_frames_in_flight> img_avail_semaphore{
                app.device, app.device, app.device},
                render_finished_semaphore{app.device, app.device, app.device};
        std::array<vk::fence, max_frames_in_flight> fence{
                app.device, app.device, app.device};


        /// ### Drawing API

        /// #### Start the render cycle
        /**
         * Returns the current frame index. This is the index between zero and
         * `max_frames_in_flight` that is currently being worked on.
         */
        felspar::coro::task<std::size_t> start(VkClearValue);

        /// #### Bind graphics pipeline
        render_parameters bind(vk::graphics_pipeline &);

        /// #### Submit and present the frame
        /// This blocks until the frame is complete
        void submit_and_present();


        /// ### View space mapping

        /// #### Reset the view matrix for world coordinates
        /**
         * Sets the matrix used by the GPU's world coordinate space transform to
         * get to the Vulkan coordinate space.
         *
         * To correct for aspect ratio and to get the Y axis in the more usual
         * direction the matrix set here must be combined properly with the
         * matrix from the `logical_vulkan_space`.
         */
        void reset_world_coordinates(affine::matrix3d const &);

        /// #### Transformation into and out of pixel coordinate space
        /**
         * Maps between the Vulkan coordinate space and pixel coordinates. Pixel
         * coordinates have their origin in the top left with the X axis running
         * right and the Y axis running down. It matches the pixel layout of the
         * screen.
         *
         * The `into` direction is used during rendering and the `outof`
         * direction can be used for mapping mouse coordinates to Vulkan ones.
         */
        affine::transform2d screen_space;


        /// ### Transformation into and out of corrected Vulkan space
        /**
         * The Vulkan coordinate system maps both width and height to -1 to +1.
         * This corrects the width and height so that the narrowest dimension is
         * in the range -1 to +1 and the widest is adjusted to give a square
         * aspect.
         */
        affine::transform2d logical_vulkan_space;


        /// ### Wait for the next render cycle
        /**
         * Waits until all frames have gone through the render cycle.
         *
         * This is useful if there is a resource being used by the current
         * frame, it can be freed after a full cycle has been completed.
         */
        struct render_cycle_awaitable {
            engine::renderer &renderer;

            bool await_ready() const noexcept { return false; }
            void await_suspend(felspar::coro::coroutine_handle<>) const;
            void await_resume() const noexcept {}
        };
        render_cycle_awaitable full_render_cycle() { return {*this}; }


      private:
        /// ### Data we need to track whilst in the render loop
        std::uint32_t image_index = {};


        /// ### View port transformation matrix and UBO

        struct coordinate_space {
            coordinate_space(renderer &rp)
            : world{rp.logical_vulkan_space.into()},
              screen{rp.screen_space.into()} {}

            affine::matrix3d world;
            affine::matrix3d screen;
        };
        coordinate_space coordinates{*this};
        std::array<buffer<coordinate_space>, max_frames_in_flight>
                viewport_buffer;
        std::array<device_memory::mapping, max_frames_in_flight> viewport_mapping;

        vk::descriptor_pool ubo_pool{app.device, max_frames_in_flight};
        vk::descriptor_sets ubo_sets{
                ubo_pool, ubo_layout, max_frames_in_flight};


        std::array<
                std::vector<felspar::coro::coroutine_handle<>>,
                max_frames_in_flight + 1>
                render_cycle_coroutines;
    };


    /// ## Create a graphics pipeline
    graphics_pipeline create_graphics_pipeline(
            app &,
            std::string_view vert,
            std::string_view frag,
            std::span<VkVertexInputBindingDescription const>,
            std::span<VkVertexInputAttributeDescription const>,
            view<vk::swap_chain>,
            view<vk::render_pass>,
            pipeline_layout,
            blend_mode = blend_mode::multiply);


}

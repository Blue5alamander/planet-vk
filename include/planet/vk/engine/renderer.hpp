#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/colour_attachment.hpp>
#include <planet/vk/engine/depth_buffer.hpp>
#include <planet/vk/engine/forward.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/engine/ubo.hpp>

#include <planet/affine/matrix3d.hpp>


namespace planet::vk::engine {


    /// ## Renderer
    class renderer final {
        std::size_t current_frame = {};
        vk::render_pass create_render_pass();

      public:
        renderer(engine::app &);
        ~renderer();

        engine::app &app;


        /// ### Allocators
        /// **NB** There are also allocators on the [device](../device.hpp).

        /// #### Per-frame memory allocator
        /**
         * This allocator should be used for anything that will only last a
         * single rendered frame. In practice this means that the memory won't
         * be freed until the same frame index comes back around to be rendered.
         */
        device_memory_allocator per_frame_memory{
                "renderer_per_frame", app.device};
        /// #### Per-swap chain
        /**
         * Certain events will cause the swap chain to be re-configured. These
         * include changes in window size and certain graphics settings. This
         * allocator should be used for things like depth buffers and colour
         * attachments that are created in response to a swap chain reset.
         */
        device_memory_allocator per_swap_chain_memory{
                "renderer_per_swap_chain", app.device};


        /// ### Swap chain, command buffers and synchronisation
        vk::swap_chain swap_chain{app.device, app.window.extents()};

        vk::command_pool command_pool{app.device, app.instance.surface};
        vk::command_buffers command_buffers{command_pool, max_frames_in_flight};

        engine::colour_attachment colour_attachment{
                per_swap_chain_memory, swap_chain};
        engine::depth_buffer depth_buffer{per_swap_chain_memory, swap_chain};

        vk::render_pass render_pass{create_render_pass()};

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
        /// ##### Bind and call render on the pipeline type
        template<typename... Pipelines>
        void render(Pipelines &...p) {
            (p.render(bind(p.pipeline)), ...);
        }

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
        void reset_world_coordinates(
                affine::matrix3d const &, affine::matrix3d const &);

        /// #### Access the world and perspective transformations
        affine::matrix3d const &world_coordinates() const noexcept {
            return coordinates.current.world;
        }
        affine::matrix3d const &perspective_projection() const noexcept {
            return coordinates.current.perspective;
        }

        /// #### The UBO descriptor layout for coordinates
        descriptor_set_layout const &coordinates_ubo_layout() const {
            return coordinates.ubo_layout;
        }


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
            felspar::coro::coroutine_handle<> mine = {};


            render_cycle_awaitable(engine::renderer &r) : renderer{r} {}
            render_cycle_awaitable(render_cycle_awaitable &&rca)
            : renderer{rca.renderer}, mine{std::exchange(rca.mine, {})} {}
            ~render_cycle_awaitable();


            bool await_ready() const noexcept { return false; }
            void await_suspend(felspar::coro::coroutine_handle<>);
            void await_resume() const noexcept {}
        };
        render_cycle_awaitable full_render_cycle() { return {*this}; }


        /// ### Wait for the next frame to cycle
        /**
         * This happens when the fences have cleared from the previous time this
         * frame index was used. Returns the frame index that is about to be
         * drawn.
         *
         * Draw commands **must not** be issued in response to waking up at this
         * point in time. Memory that has been in use for rendering _may_ be
         * freed at this point in time, but only for the frame index returned.
         */
        struct render_prestart_awaitable {
            engine::renderer &renderer;
            felspar::coro::coroutine_handle<> mine = {};


            render_prestart_awaitable(engine::renderer &r) : renderer{r} {}
            render_prestart_awaitable(render_prestart_awaitable &&rpa)
            : renderer{rpa.renderer}, mine{std::exchange(rpa.mine, {})} {}
            ~render_prestart_awaitable();


            bool await_ready() const noexcept { return false; }
            void await_suspend(felspar::coro::coroutine_handle<>);
            std::size_t await_resume() const noexcept;
        };
        render_prestart_awaitable next_frame_prestart() { return {*this}; }


      private:
        /// ### Data we need to track whilst in the render loop
        std::uint32_t image_index = {};


        /// ### Re-create the swap chain
        void recreate_swap_chain(
                VkResult,
                felspar::source_location const & =
                        felspar::source_location::current());


        /// ### View port transformation matrix and UBO

        struct coordinate_space {
            coordinate_space(renderer &rp)
            : world{},
              screen{rp.screen_space.into()},
              perspective{rp.logical_vulkan_space.into()} {}


            affine::matrix3d world;
            affine::matrix3d screen;
            affine::matrix3d perspective = {};


            void copy_to_gpu_memory(std::byte *memory) const {
                std::memcpy(memory, this, sizeof(coordinate_space));
            }
        };
        ubo<coordinate_space> coordinates;


        /// TODO This array would be better as a circular buffer
        std::array<
                std::vector<felspar::coro::coroutine_handle<>>,
                max_frames_in_flight + 1>
                render_cycle_coroutines;
        std::vector<felspar::coro::coroutine_handle<>> pre_start_coroutines;
    };


    /// ## Create a graphics pipeline
    struct graphics_pipeline_parameters {
        engine::app &app;
        engine::renderer &renderer;

        std::string_view vertex_shader;
        std::string_view fragment_shader;
        std::span<VkVertexInputBindingDescription const> binding_descriptions;
        std::span<VkVertexInputAttributeDescription const> attribute_descriptions;

        VkExtent2D extents = renderer.swap_chain.extents;
        view<vk::render_pass> render_pass = renderer.render_pass;

        engine::blend_mode blend_mode = blend_mode::multiply;
        std::size_t sub_pass = 0;

        vk::pipeline_layout pipeline_layout;
    };
    graphics_pipeline create_graphics_pipeline(graphics_pipeline_parameters);


}

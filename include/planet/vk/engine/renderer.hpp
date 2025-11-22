#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/array.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/depth_buffer.hpp>
#include <planet/vk/frame_buffer.hpp>
#include <planet/vk/engine/postprocess/glow.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/ubo/coordinate_space.hpp>


namespace planet::vk::engine {


    /// ## Renderer
    class renderer final {
        std::size_t fif_image_index = {};
        /**
         * This is the frame-in-flight index we're currently rendering.
         * Frames-in-flight are worked through one at a time sequentially.
         */


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


        /// #### Attachments and frame buffers
        engine::colour_attachment colour_attachments;
        engine::depth_buffer depth_buffers;
        engine::colour_attachment scene_colours;


        /// #### Post-process pipeline
        postprocess::glow postprocess;


        /// #### Render passes
        vk::render_pass scene_render_pass;
        std::array<frame_buffer, max_frames_in_flight> scene_frame_buffers;


        /// #### Synchronisation
        std::array<vk::semaphore, max_frames_in_flight> img_avail_semaphore{
                array_of<max_frames_in_flight>(
                        [this]() { return vk::semaphore{app.device}; })};
        std::vector<vk::semaphore> render_finished_semaphore;
        std::array<vk::fence, max_frames_in_flight> fence{
                array_of<max_frames_in_flight>(
                        [this]() { return vk::fence{app.device}; })};


        /// ### Drawing API

        /// #### Start the render cycle
        /**
         * Returns the current frame index. This is the index between zero and
         * `max_frames_in_flight` that is currently being worked on.
         */
        felspar::coro::task<std::size_t> start(VkClearValue);
        auto start(planet::colour const &colour) {
            return start(as_VkClearValue(colour));
        }


        /// #### Bind graphics pipeline
        render_parameters
                bind(vk::graphics_pipeline &,
                     std::span<ubo::coherent_details const *const>);
        /// ##### Bind and call render on the pipeline type
        template<typename... Shaders>
        void render(Shaders &&...s) {
            (bind_and_render(std::forward<Shaders>(s)), ...);
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
        void reset_world_coordinates(
                affine::matrix3d const &, affine::point3d const &);
        void reset_world_coordinates(
                affine::matrix3d const &,
                affine::matrix3d const &,
                affine::point3d const &);

        /// #### Access the world and perspective transformations
        affine::matrix3d const &world_coordinates() const noexcept {
            return coordinates.current.world;
        }
        affine::matrix3d const &perspective_projection() const noexcept {
            return coordinates.current.perspective;
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


        /// ### Memory coherent UBOs
        std::span<ubo::coherent_details const *const>
                default_coherent_ubos() const {
            return m_default_coherent_ubos;
        }

        /// #### The UBO descriptor layout for coordinates
        ubo::coherent_details const &coordinates_ubo_details() const {
            return coordinates.vk;
        }
        descriptor_set_layout const &coordinates_ubo_layout() const {
            return coordinates.vk.layout;
        }


        /// ### Wait for the next render cycle
        /**
         * Waits until all frames have gone through the render cycle.
         *
         * This is useful if there is a resource being used by the current
         * frame, it can be freed after a full cycle has been completed.
         */
        struct render_cycle_awaitable {
            engine::renderer &renderer;
            std::coroutine_handle<> mine = {};


            render_cycle_awaitable(engine::renderer &r) : renderer{r} {}
            render_cycle_awaitable(render_cycle_awaitable &&rca)
            : renderer{rca.renderer}, mine{std::exchange(rca.mine, {})} {}
            ~render_cycle_awaitable();


            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<>);
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
            std::coroutine_handle<> mine = {};


            render_prestart_awaitable(engine::renderer &r) : renderer{r} {}
            render_prestart_awaitable(render_prestart_awaitable &&rpa)
            : renderer{rpa.renderer}, mine{std::exchange(rpa.mine, {})} {}
            ~render_prestart_awaitable();


            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<>);
            std::size_t await_resume() const noexcept;
        };
        render_prestart_awaitable next_frame_prestart() { return {*this}; }


      private:
        /// ### Data we need to track whilst in the render loop
        std::uint32_t scfb_image_index = {};
        /**
         * This is the index into the swap chain frame buffers that the GPU has
         * told us to use to record commands and submit for the next frame to
         * present to the user.
         */


        /// ### Re-create the swap chain
        void recreate_swap_chain(
                VkResult,
                std::source_location const & = std::source_location::current());
        bool swap_chain_suboptimal = false;


        /// ### Standard UBOs
        ubo::coordinate_space::ubo_type<max_frames_in_flight> coordinates;
        std::array<ubo::coherent_details const *const, 1>
                m_default_coherent_ubos{&coordinates.vk};

        /// #### Binds and renders a shader
        template<typename Shader, std::size_t N>
        void bind_and_render(
                std::pair<
                        Shader &,
                        std::span<
                                planet::vk::ubo::coherent_details const *const,
                                N>> &&p) {
            p.first.render(bind(p.first.pipeline, p.second));
        }
        template<typename Shader>
        void bind_and_render(Shader &s) {
            s.render(bind(s.pipeline, find_coherent_details(s, *this)));
        }
        /// #### Finds the coherent memory UBOs used by the shader
        /**
         * Any shader that uses anything different to the renderer's default
         * memory coherent UBOs needs to implement `coherent_details_for` which
         * returns the span of `ubo::coherent_details` pointers that are to be
         * bound for the pipeline in the shader.
         */
        template<typename Shader>
        std::span<ubo::coherent_details const *const>
                find_coherent_details(Shader &s, renderer &r) {
            if constexpr (requires { coherent_details_for(s); }) {
                return coherent_details_for(s);
            } else {
                return r.default_coherent_ubos();
            }
        }


        /// TODO This array would be better as a circular buffer
        std::array<std::vector<std::coroutine_handle<>>, max_frames_in_flight + 1>
                render_cycle_coroutines;
        std::vector<std::coroutine_handle<>> pre_start_coroutines;
    };


    /// ## Create a graphics pipeline
    struct graphics_pipeline_parameters {
        engine::app &app;
        engine::renderer &renderer;

        shader_parameters vertex_shader, fragment_shader;
        std::span<VkVertexInputBindingDescription const> binding_descriptions;
        std::span<VkVertexInputAttributeDescription const> attribute_descriptions;
        std::uint32_t colour_attachments = 2;

        VkExtent2D extents = renderer.swap_chain.extents;
        view<vk::render_pass> render_pass = renderer.scene_render_pass;

        bool write_to_depth_buffer = true;
        VkSampleCountFlagBits multisampling = app.instance.gpu().msaa_samples;
        engine::blend_mode blend_mode = blend_mode::multiply;
        std::size_t sub_pass = 0;

        vk::pipeline_layout pipeline_layout;
    };
    graphics_pipeline create_graphics_pipeline(
            graphics_pipeline_parameters,
            std::source_location const & = std::source_location::current());


}

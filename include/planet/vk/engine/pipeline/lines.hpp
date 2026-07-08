#pragma once


#include <planet/affine/point2d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/vertex/coloured.hpp>


namespace planet::vk::engine::pipeline {


    /// ## 2D coloured line list
    /**
     * Draws independent coloured line segments using
     * `VK_PRIMITIVE_TOPOLOGY_LINE_LIST`. It reuses the mesh vertex format
     * (`vertex::coloured`) and the mesh shaders unchanged -- only the pipeline
     * topology differs.
     */
    class lines final {
      public:
        /// ### Construction
        /**
         * Always supply the renderer. The vertex and fragment shader default
         * to the mesh shaders, the blend mode to `multiply` and the pipeline
         * layout to the renderer's coordinates UBO layout.
         */
        struct parameters {
            engine::renderer &renderer;
            shader_parameters vertex_shader{
                    .spirv_filename = "planet-vk-engine/mesh.world.vert.spirv"};
            shader_parameters fragment_shader{
                    .spirv_filename = "planet-vk-engine/mesh.frag.spirv"};
            engine::blend_mode blend_mode = engine::blend_mode::multiply;
            pipeline_layout layout{
                    renderer.app.device, renderer.coordinates_ubo_layout()};
        };
        lines(parameters);


        vk::graphics_pipeline pipeline;


        /// ### Line data to be drawn
        class data {
            friend class lines;
            std::vector<vertex::coloured> vertices;
            std::vector<std::uint32_t> indices;


          public:
            void clear() {
                vertices.clear();
                indices.clear();
            }
            [[nodiscard]] bool empty() const noexcept {
                return vertices.empty();
            }


            /// #### 2D Z layer height
            float z_layer = 0.0f;


            /// #### Draw a raw line list
            void
                    draw(std::span<vertex::coloured const>,
                         std::span<std::uint32_t const>);
            /// #### Draw a single line segment between two points
            void
                    segment(affine::point2d const &,
                            affine::point2d const &,
                            colour const &);
            /// #### Draw a closed loop through the given points
            /**
             * Emits the `{0,1, 1,2, ..., n-1, 0}` `LINE_LIST` indices, drawing
             * a segment between each consecutive pair of points and a final
             * segment closing the loop back to the first point.
             */
            void closed_loop(std::span<affine::point2d const>, colour const &);
        };


        /// ### This frame's draw data and commands
        data this_frame;
        /// #### Draw already captured data
        void draw(data const &);


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        std::array<buffer<vertex::coloured>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

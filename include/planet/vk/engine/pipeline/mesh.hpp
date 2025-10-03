#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/vertex/coloured.hpp>


namespace planet::vk::engine::pipeline {


    /// ## 2D triangle mesh with per-vertex colour
    class mesh final {
        static pipeline_layout default_layout(engine::renderer &);


      public:
        /// ### Construction
        /**
         * Always supply the renderer and the filename of the vertex shader, but
         * the fragment shader, blend mode and the pipeline layout are optional.
         *
         * If a fragment shader is provided then the vertex must also be
         * provided as the first `std::string_view` passed in is taken as the
         * vertex shader.
         */
        struct parameters {
            engine::renderer &renderer;
            shader_parameters vertex_shader;
            shader_parameters fragment_shader{
                    .spirv_filename = "planet-vk-engine/mesh.frag.spirv"};
            blend_mode bm = blend_mode::multiply;
            pipeline_layout layout{
                    renderer.app.device, renderer.coordinates_ubo_layout()};
        };
        mesh(parameters);


        vk::graphics_pipeline pipeline;


        /// ### Mesh data to be drawn
        class data {
            friend class mesh;
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
            float z_layer = 0.75f;


            /// #### Draw a triangle mesh
            void
                    draw(std::span<vertex::coloured const>,
                         std::span<std::uint32_t const>);
            void
                    draw(std::span<vertex::coloured const>,
                         std::span<std::uint32_t const>,
                         planet::affine::point3d const &);
            void
                    draw(std::span<vertex::coloured const>,
                         std::span<std::uint32_t const>,
                         planet::affine::point3d const &,
                         colour const &);
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

#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/vertex/coloured.hpp>


namespace planet::vk::engine::pipeline {


    /// ## 2D triangle mesh with per-vertex colour
    class mesh final {
        static pipeline_layout default_layout(engine::renderer &);


      public:
        static constexpr std::string_view default_fragment_shader =
                "planet-vk-engine/mesh.frag.spirv";
        static constexpr blend_mode default_blend_mode = blend_mode::multiply;


        /// ### Construction
        /**
         * Always supply the renderer and the filename of the vertex shader, but
         * the fragment shader, blend mode and the pipeline layout are optional.
         *
         * If a fragment shader is provided then the vertex must also be
         * provided as the first `std::string_view` passed in is taken as the
         * vertex shader.
         */
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             blend_mode const bm = default_blend_mode)
        : mesh{r, vertex_spirv, default_fragment_shader, bm,
               default_layout(r)} {}
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             pipeline_layout layout)
        : mesh{r, vertex_spirv, default_fragment_shader, default_blend_mode,
               std::move(layout)} {}
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             std::string_view const fragment_spirv)
        : mesh{r, vertex_spirv, fragment_spirv, default_blend_mode,
               default_layout(r)} {}
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             std::string_view const fragment_spirv,
             pipeline_layout layout)
        : mesh{r, vertex_spirv, fragment_spirv, default_blend_mode,
               std::move(layout)} {}
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             blend_mode const bm,
             pipeline_layout layout)
        : mesh{r, vertex_spirv, default_fragment_shader, bm,
               std::move(layout)} {}
        mesh(engine::renderer &,
             std::string_view vertex_spirv,
             std::string_view fragment_spirv,
             blend_mode,
             pipeline_layout);


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

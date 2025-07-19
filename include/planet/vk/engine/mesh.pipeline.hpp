#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## 2D triangle mesh with per-vertex colour
    class mesh final {
        static pipeline_layout default_layout(engine::renderer &);


      public:
        /// ### Construction
        /**
         * Always supply the renderer and the filename of the vertex shader, but
         * the blend mode and the pipeline layout are both optional.
         */
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             blend_mode const bm = blend_mode::multiply)
        : mesh{r, vertex_spirv, bm, default_layout(r)} {}
        mesh(engine::renderer &r,
             std::string_view const vertex_spirv,
             pipeline_layout layout)
        : mesh{r, vertex_spirv, blend_mode::multiply, std::move(layout)} {}
        mesh(engine::renderer &,
             std::string_view vertex_spirv,
             blend_mode,
             pipeline_layout);


        vk::graphics_pipeline pipeline;


        struct vertex {
            planet::affine::point3d p;
            colour c;
        };


        /// ### Mesh data to be drawn
        class data {
            friend class mesh;
            std::vector<vertex> vertices;
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
            void draw(std::span<vertex const>, std::span<std::uint32_t const>);
            void
                    draw(std::span<vertex const>,
                         std::span<std::uint32_t const>,
                         planet::affine::point3d const &);
            void
                    draw(std::span<vertex const>,
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
        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

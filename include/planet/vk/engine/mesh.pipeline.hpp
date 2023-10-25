#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## 2D triangle mesh with per-vertex colour
    class mesh final {
      public:
        mesh(engine::renderer &,
             std::string_view vertex_spirv,
             blend_mode = blend_mode::multiply);

        engine::app &app;
        view<vk::swap_chain> swap_chain;
        view<vk::render_pass> render_pass;

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


            /// #### Draw a 2D triangle mesh with an optional positional offset
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


        /// ### Draw captured mesh data
        void draw(data const &);

        /// #### Forward to the internal mesh data
        void
                draw(std::span<vertex const> const v,
                     std::span<std::uint32_t const> const i) {
            draw_data.draw(v, i);
        }
        void
                draw(std::span<vertex const> const v,
                     std::span<std::uint32_t const> const i,
                     planet::affine::point3d const &o) {
            draw_data.draw(v, i, o);
        }
        void
                draw(std::span<vertex const> const v,
                     std::span<std::uint32_t const> const i,
                     planet::affine::point3d const &o,
                     colour const &c) {
            draw_data.draw(v, i, o, c);
        }


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        data draw_data;
        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;

        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

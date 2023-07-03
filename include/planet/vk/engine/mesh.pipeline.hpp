#pragma once


#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## 2D triangle mesh with per-vertex colour
    class mesh final {
        vk::graphics_pipeline create_mesh_pipeline(std::string_view, blend_mode);

      public:
        mesh(engine::app &,
             engine::renderer &,
             blend_mode = blend_mode::multiply);
        mesh(engine::app &,
             engine::renderer &,
             std::string_view vertex_spirv,
             blend_mode = blend_mode::multiply);
        mesh(engine::app &,
             vk::swap_chain &,
             vk::render_pass &,
             vk::descriptor_set_layout &,
             blend_mode = blend_mode::multiply);
        mesh(engine::app &,
             vk::swap_chain &,
             vk::render_pass &,
             vk::descriptor_set_layout &,
             std::string_view vertex_spirv,
             blend_mode = blend_mode::multiply);

        engine::app &app;
        view<vk::swap_chain> swap_chain;
        view<vk::render_pass> render_pass;
        view<vk::descriptor_set_layout> ubo_layout;

        vk::graphics_pipeline pipeline;


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


        /// ### Draw a 2D triangle mesh with an optional positional offset
        void draw(std::span<vertex const>, std::span<std::uint32_t const>);
        void draw(std::span<vertex const>, std::span<std::uint32_t const>, pos);
        void
                draw(std::span<vertex const>,
                     std::span<std::uint32_t const>,
                     pos,
                     colour const &);


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        std::vector<vertex> triangles;
        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;

        std::vector<std::uint32_t> indexes;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

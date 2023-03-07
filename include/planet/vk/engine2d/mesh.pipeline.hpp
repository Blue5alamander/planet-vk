#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d::pipeline {


    /// ## 2D triangle mesh with per-vertex colour
    class mesh final {
        vk::graphics_pipeline create_mesh_pipeline();

      public:
        mesh(engine2d::app &,
             vk::swap_chain &,
             vk::render_pass &,
             vk::descriptor_set_layout &);

        engine2d::app &app;
        view<vk::swap_chain> swap_chain;
        view<vk::render_pass> render_pass;
        view<vk::descriptor_set_layout> ubo_layout;

        vk::graphics_pipeline pipeline{create_mesh_pipeline()};


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
        void render(renderer &, command_buffer &, std::size_t current_frame);

      private:
        std::vector<vertex> triangles;
        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;

        std::vector<std::uint32_t> indexes;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

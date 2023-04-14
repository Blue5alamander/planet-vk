#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d::pipeline {


    /// ## Textured triangle pipeline
    class textured final {
        graphics_pipeline create_pipeline();

      public:
        textured(
                engine2d::app &,
                vk::swap_chain &,
                vk::render_pass &,
                vk::descriptor_set_layout &);

        engine2d::app &app;
        view<vk::swap_chain> swap_chain;
        view<vk::render_pass> render_pass;
        view<vk::descriptor_set_layout> vp_layout;
        vk::descriptor_set_layout texture_layout;

        vk::graphics_pipeline pipeline{create_pipeline()};


        struct pos {
            float x = {}, y = {};
            friend constexpr pos operator+(pos const l, pos const r) {
                return {l.x + r.x, l.y + r.y};
            }
        };
        struct vertex {
            pos p, uv;
            colour col = {1.0f, 1.0f, 1.0f};
        };


        static constexpr std::size_t max_textures_per_frame = 10240;

        vk::descriptor_pool texture_pool{
                app.device, max_frames_in_flight *max_textures_per_frame};
        std::array<vk::descriptor_sets, max_frames_in_flight> texture_sets;

        std::vector<vertex> quads;
        std::vector<std::uint32_t> indexes;
        std::vector<VkDescriptorImageInfo> textures;

        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;

        /// ### Drawing API

        /// #### Draw texture stretched to the axis aligned rectangle
        void draw(vk::texture const &, affine::rectangle2d);


        /// ### Add draw commands to command buffer
        void render(renderer &, command_buffer &, std::size_t current_frame);
    };


}

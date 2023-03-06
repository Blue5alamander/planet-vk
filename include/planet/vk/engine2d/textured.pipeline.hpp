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
            std::uint32_t tex_id = {};
        };


        static constexpr std::size_t max_textures_per_frame = 1024;

        vk::descriptor_pool texture_pool{app.device, max_frames_in_flight};
        vk::descriptor_sets texture_sets{
                texture_pool, texture_layout, max_frames_in_flight};

        std::vector<vertex> quads;
        std::vector<VkDescriptorImageInfo> textures;
    };


}

#pragma once


#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Textured triangle pipeline
    class textured final {
        graphics_pipeline create_pipeline(std::string_view vertex_shader);

      public:
        textured(
                engine::app &,
                vk::swap_chain &,
                vk::render_pass &,
                vk::descriptor_set_layout &,
                std::string_view vertex_shader);

        engine::app &app;
        view<vk::swap_chain> swap_chain;
        view<vk::render_pass> render_pass;
        view<vk::descriptor_set_layout> vp_layout;
        vk::descriptor_set_layout texture_layout;

        vk::graphics_pipeline pipeline;


        struct pos {
            float x = {}, y = {};
            friend constexpr pos operator+(pos const l, pos const r) {
                return {l.x + r.x, l.y + r.y};
            }
        };
        struct vertex {
            pos p, uv;
            colour col = white;
        };


        /// ### Texture data to be drawn
        class data {
            friend class textured;
            std::vector<vertex> vertices;
            std::vector<std::uint32_t> indices;
            std::vector<VkDescriptorImageInfo> textures;


          public:
            void clear() {
                vertices.clear();
                indices.clear();
                textures.clear();
            }
            [[nodiscard]] bool empty() const noexcept {
                return vertices.empty();
            }


            /// ### Drawing API

            /// #### Draw texture stretched to the axis aligned rectangle
            void
                    draw(vk::texture const &,
                         affine::rectangle2d const &,
                         colour const & = white);
        };


        /// ### Draw captured textured data
        void draw(data const &);

        /// #### Forward to the internal textured data
        void
                draw(vk::texture const &tx,
                     affine::rectangle2d const &r,
                     colour const &c = white) {
            draw_data.draw(tx, r, c);
        }


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        static constexpr std::size_t max_textures_per_frame = 10240;

        vk::descriptor_pool texture_pool{
                app.device, max_frames_in_flight *max_textures_per_frame};
        std::array<vk::descriptor_sets, max_frames_in_flight> texture_sets;

        data draw_data;

        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

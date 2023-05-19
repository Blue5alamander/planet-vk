#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d::pipeline {


    /// ## Textured triangle pipeline
    class sprite final {
        graphics_pipeline create_pipeline(std::string_view vertex_shader);

      public:
        sprite(engine2d::app &,
               vk::swap_chain &,
               vk::render_pass &,
               vk::descriptor_set_layout &,
               std::string_view vertex_shader);

        engine2d::app &app;
        view<vk::swap_chain> swap_chain;
        view<vk::render_pass> render_pass;
        view<vk::descriptor_set_layout> vp_layout;
        vk::descriptor_set_layout texture_layout;

        vk::graphics_pipeline pipeline;

        struct push_constant {
            affine::matrix3d transform;
        };

        /// ### Position for a sprite
        struct location {
            /// #### Positioning and scale

            /// ##### Offset
            /// The world location of the sprite
            planet::affine::point2d offset{};

            /// ##### Size
            /**
             * The size is not axis aligned and is in world co-ordinate space
             * units. The aspect ratio provided here should account for any
             * correction between the underlying texture's aspect ratio and the
             * world co-ordinate aspect ratio.
             */
            planet::affine::extents2d size{};
            /// ##### Rotation centre of the sprite
            /**
             * The offset is from the bottom left of the sprite with x-axis left
             * and y-axis up
             */
            planet::affine::point2d centre{size.width / 2, size.height / 2};
            /// ##### Scale
            float scale = {1};
            /// ##### Rotation
            /// Measured in anti-clockwise turns
            float rotation = {};
        };

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
        void draw(vk::texture const &, location const &, colour const & = white);


        /// ### Add draw commands to command buffer
        void render(renderer &, command_buffer &, std::size_t current_frame);
    };


}
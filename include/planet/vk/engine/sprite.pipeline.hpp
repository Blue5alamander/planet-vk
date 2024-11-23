#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/affine/point3d.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Draw sprites
    /**
     * A sprite is a texture drawn on a quad at a particular location with a
     * provided rotation about a provided centre. These are typically used for
     * game characters, shots etc. that need to be drawn in world coordinate
     * space.
     */
    class sprite final : private telemetry::id {
        graphics_pipeline create_pipeline(engine::renderer &, std::string_view);

      public:
        sprite(engine::renderer &r,
               std::string_view const vertex_shader,
               std::uint32_t const textures_per_frame = 256)
        : sprite{"planet_vk_engine_pipeline_sprite", r, vertex_shader,
                 textures_per_frame, id::suffix::yes} {}
        sprite(std::string_view,
               engine::renderer &,
               std::string_view vertex_shader,
               std::uint32_t textures_per_frame = 256,
               id::suffix = id::suffix::no);

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
            /// ##### Z height
            float z_height = {};
        };

        struct pos {
            float x = {}, y = {};
            friend constexpr pos operator+(pos const l, pos const r) {
                return {l.x + r.x, l.y + r.y};
            }
        };
        struct vertex {
            planet::affine::point3d p;
            colour col = colour::white;
            pos uv;
        };


        /// ### The maximum number of textures
        /**
         * Used to size the descriptor pool and the descriptor sets. The number
         * of individual textures cannot be higher than this number.
         */
        std::uint32_t max_textures_per_frame;

        vk::descriptor_pool texture_pool;
        std::array<vk::descriptor_sets, max_frames_in_flight> texture_sets;

        std::vector<vertex> quads;
        std::vector<std::uint32_t> indexes;
        std::vector<VkDescriptorImageInfo> textures;
        std::vector<push_constant> transforms;

        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;


        /// ### Drawing API

        /// #### Draw texture stretched to the axis aligned rectangle
        void
                draw(std::pair<vk::texture const &, affine::rectangle2d>,
                     location const &,
                     colour const & = colour::white);
        void
                draw(vk::texture const &t,
                     location const &l,
                     colour const &c = colour::white) {
            draw({t, {{0, 0}, affine::extents2d{1, 1}}}, l, c);
        }


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        /// ### Performance counters
        telemetry::max textures_in_frame;
    };


}

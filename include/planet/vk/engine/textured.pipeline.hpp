#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Textured triangle pipeline
    class textured final : private telemetry::id {
      public:
        textured(
                engine::renderer &r,
                std::string_view const vertex_shader,
                std::uint32_t const textures_per_frame = 256)
        : textured{
                  "planet_vk_engine_pipeline_textured", r, vertex_shader,
                  textures_per_frame, id::suffix::yes} {}
        textured(
                std::string_view,
                engine::renderer &,
                std::string_view vertex_shader,
                std::uint32_t textures_per_frame = 256,
                id::suffix = id::suffix::no);

        vk::descriptor_set_layout texture_layout;
        vk::graphics_pipeline pipeline;


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


            /// #### 2D Z layer height
            float z_layer = 0.75f;


            /// ### Drawing API

            /// #### Draw texture stretched to the axis aligned rectangle
            void
                    draw(vk::texture const &t,
                         affine::rectangle2d const &r,
                         colour const &c,
                         float const z) {
                draw({t, {{0, 0}, affine::extents2d{1, 1}}}, r, c, z);
            }
            void
                    draw(vk::texture const &t,
                         affine::rectangle2d const &r,
                         colour const &c = colour::white) {
                draw({t, {{0, 0}, affine::extents2d{1, 1}}}, r, c, z_layer);
            }
            void
                    draw(vk::sub_texture const &,
                         affine::rectangle2d const &,
                         colour const &,
                         float z);
            void
                    draw(vk::sub_texture const &t,
                         affine::rectangle2d const &r,
                         colour const &c = colour::white) {
                draw(t, r, c, z_layer);
            }
            /// #### Draw a textured mesh
            void
                    draw(std::span<vertex const>,
                         std::span<std::uint32_t const>,
                         std::span<VkDescriptorImageInfo const>);
            void draw(data const &d) {
                draw(d.vertices, d.indices, d.textures);
            }
        };


        /// ### This frame's draw data and commands
        data this_frame;


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        std::uint32_t max_textures_per_frame;

        vk::descriptor_pool texture_pool;
        std::array<vk::descriptor_sets, max_frames_in_flight> texture_sets;

        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;


        /// ### Performance counters
        telemetry::max textures_in_frame;
    };


}

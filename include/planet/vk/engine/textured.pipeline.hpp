#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Textured triangle pipeline
    class textured final {
      public:
        textured(engine::renderer &, std::string_view vertex_shader);

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
            colour col = white;
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


            /// ### Drawing API

            /// #### Draw texture stretched to the axis aligned rectangle
            void
                    draw(vk::texture const &t,
                         affine::rectangle2d const &r,
                         colour const &c = white,
                         float z = {}) {
                draw({t, {{0, 0}, affine::extents2d{1, 1}}}, r, c, z);
            }
            void
                    draw(std::pair<vk::texture const &, affine::rectangle2d>,
                         affine::rectangle2d const &,
                         colour const & = white,
                         float z = {});
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
        static constexpr std::size_t max_textures_per_frame = 10240;

        vk::descriptor_pool texture_pool;
        std::array<vk::descriptor_sets, max_frames_in_flight> texture_sets;

        std::array<buffer<vertex>, max_frames_in_flight> vertex_buffers;
        std::array<buffer<std::uint32_t>, max_frames_in_flight> index_buffers;
    };


}

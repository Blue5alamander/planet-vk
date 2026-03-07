#pragma once


#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/memory/pooled-vector-map.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/ubo/textures.hpp>
#include <planet/vk/vertex/coloured_textured.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Textured quad pipeline
    /**
     * Draws 2D axis-aligned quads with texture support. Quads are grouped by
     * texture to reduce the number of texture bindings and descriptor updates.
     */
    class textured_quad final : private telemetry::id {
      public:
        using vertex_type = vertex::coloured_textured;


        struct parameters {
            static std::string_view constexpr default_name =
                    "planet_vk_engine_pipeline_textured_quad";

            std::string_view name = default_name;
            id::suffix use_name_suffix =
                    (name == default_name ? id::suffix::add
                                          : id::suffix::suppress);
            engine::renderer &renderer;
            shader_parameters vertex_shader;
            shader_parameters fragment_shader{
                    .spirv_filename = "planet-vk-engine/textured.frag.spirv"};
            std::uint32_t const textures_per_frame = 256;
        };
        textured_quad(parameters);


        ubo::textures<vertex_type, engine::max_frames_in_flight> textures_ubo;
        vk::graphics_pipeline pipeline;


        /// ### Texture data to be drawn

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


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        struct quad_draw_info {
            affine::rectangle2d position;
            affine::rectangle2d uv;
            colour colour;
            float z;
        };

        planet::vk::engine::memory::pooled_vector_map<
                std::map<vk::texture const *, std::vector<quad_draw_info>>>
                commands;

        std::vector<vertex_type> vertices;
        std::vector<std::uint32_t> indices;
        std::array<buffer<vertex_type>, engine::max_frames_in_flight>
                vertex_buffers;
        std::array<buffer<std::uint32_t>, engine::max_frames_in_flight>
                index_buffers;
    };


}

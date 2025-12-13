#pragma once


#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/engine/textured.draw.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Textured triangle pipeline
    /**
     * The current implementation assumes that only quads are being drawn. To
     * fix this it will need to save how many vertices/indices are drawn per
     * texture so it can dispatch the correct number per draw call after binding
     * the texture.
     */
    class textured final : private telemetry::id {
      public:
        using textures_type = draw_basic_textures<>;


        struct parameters {
            static std::string_view constexpr default_name =
                    "planet_vk_engine_pipeline_textured";

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
        textured(parameters);


        textures_type textures;
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
    };


}

#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/ubo/textures.hpp>


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


        vk::textures<max_frames_in_flight> textures;
        vk::graphics_pipeline pipeline;


        /// ### Texture data to be drawn
        std::vector<vk::textures<max_frames_in_flight>::vertex> vertices;
        std::vector<std::uint32_t> indices;


        void clear() {
            vertices.clear();
            indices.clear();
            textures.descriptors.clear();
        }
        [[nodiscard]] bool empty() const noexcept { return vertices.empty(); }


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
                draw(std::span<vk::textures<max_frames_in_flight>::vertex const>,
                     std::span<std::uint32_t const>,
                     std::span<VkDescriptorImageInfo const>);


        /// ### Add draw commands to command buffer
        void render(render_parameters);


      private:
        /// ### Performance counters
        telemetry::max textures_in_frame;
    };


}

#pragma once


#include <planet/telemetry/id.hpp>
#include <planet/vk/descriptors.hpp>
#include <planet/vk/engine/colour_attachment.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/pipeline.hpp>
#include <planet/vk/render_pass.hpp>
#include <planet/vk/texture.hpp>


namespace planet::vk::engine::postprocess {


    /// ## Glow postprocess
    /**
     * Intended to perform post-processing in the fragment shader. The vertex
     * shader is always a single full-screen triangle that outputs position and
     * UV coordinates for sampling the input image. The default fragment shader
     * simply copies the input.
     */
    class glow final : private telemetry::id {
      public:
        struct parameters {
            engine::renderer &renderer;
        };
        glow(parameters);


        engine::app &app;
        engine::renderer &renderer;

        std::array<engine::colour_attachment, max_frames_in_flight>
                input_attachments, input_colours;

        vk::descriptor_set_layout sampler_layout;
        vk::sampler sampler;
        vk::descriptor_pool descriptor_pool;
        vk::descriptor_sets descriptor_sets;

        vk::render_pass present_render_pass;

        vk::graphics_pipeline pipeline;


        void update_descriptors();
        void render_subpass(render_parameters, std::uint32_t);
    };


}

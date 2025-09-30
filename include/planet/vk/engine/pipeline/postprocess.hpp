#pragma once


#include <planet/telemetry/id.hpp>
#include <planet/vk/descriptors.hpp>
#include <planet/vk/engine/forward.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/pipeline.hpp>
#include <planet/vk/texture.hpp>


namespace planet::vk::engine::pipeline {


    /// ## Post-processing pipeline
    /**
     * Intended to perform post-processing in the fragment shader. The vertex
     * shader is always a single full-screen triangle that outputs position and
     * UV coordinates for sampling the input image. The default fragment shader
     * simply copies the input.
     */
    class postprocess final : private telemetry::id {
      public:
        struct parameters {
            engine::renderer &renderer;
        };
        postprocess(parameters);


        vk::descriptor_set_layout sampler_layout;
        vk::sampler sampler;
        vk::descriptor_pool descriptor_pool;
        vk::descriptor_sets descriptor_sets;

        vk::graphics_pipeline pipeline;


        void update_descriptors(renderer &);
        void render_subpass(render_parameters, std::uint32_t);
    };


}

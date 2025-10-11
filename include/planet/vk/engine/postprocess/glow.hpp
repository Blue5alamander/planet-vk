#pragma once


#include <planet/telemetry/id.hpp>
#include <planet/vk/descriptors.hpp>
#include <planet/vk/engine/colour_attachment.hpp>
#include <planet/vk/engine/render_parameters.hpp>
#include <planet/vk/frame_buffer.hpp>
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

        void recreate_swap_chain();


        engine::app &app;
        engine::renderer &renderer;


        /// ### Inputs, downsize and blur
        engine::colour_attachment input_attachments, input_colours,
                downsized_input, horizontal_blur, vertical_blur;

        vk::descriptor_set_layout blur_sampler_layout;
        vk::sampler blur_sampler;
        vk::descriptor_pool blur_descriptor_pool;
        vk::descriptor_sets horizontal_descriptor_sets,
                vertical_descriptor_sets;

        vk::render_pass blur_render_pass;
        vk::graphics_pipeline horizontal_pipeline, vertical_pipeline;

        std::array<vk::frame_buffer, max_frames_in_flight>
                horizontal_frame_buffers, vertical_frame_buffers;


        /// ### Presentation
        vk::descriptor_set_layout present_sampler_layout;
        vk::sampler present_sampler;
        vk::descriptor_pool present_descriptor_pool;
        vk::descriptor_sets present_descriptor_sets;

        vk::render_pass present_render_pass;

        vk::graphics_pipeline present_pipeline;

        void render_subpass(render_parameters, std::uint32_t);

      private:
        void initial_image_transition();
        void update_descriptors();
    };


}

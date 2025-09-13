#pragma once


#include <planet/vk/commands.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine {


    /// ## Parameters needed by a graphics pipeline
    struct render_parameters {
        engine::renderer &renderer;
        command_buffer &cb;
        /// ### Frame number
        /// Always between zero and `max_frames_in_flight` - 1
        std::size_t current_frame;
    };


    /// ## Blend mode for render pipeline
    enum class blend_mode { none, multiply, add };


    /// ## Describe the shader code
    struct shader_parameters {
        std::string_view spirv_filename;
        char const *entry_point = "main";
    };


}

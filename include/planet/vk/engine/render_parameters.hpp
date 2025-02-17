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


}

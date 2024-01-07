#pragma once


#include <planet/vk/forward.hpp>

#include <cstddef>


namespace planet::vk::engine {


    /// Maximum number of frames that we're willing to deal with at any given time
    constexpr std::size_t max_frames_in_flight = 3;


    class app;
    struct depth_buffer;
    class renderer;


    namespace pipeline {


        class mesh;
        class sprite;
        class textured;


    }


    namespace ui {


        template<typename T>
        struct per_frame;
        template<
                typename Pipeline = pipeline::textured,
                typename Texture = vk::texture>
        struct tx;

    }

}

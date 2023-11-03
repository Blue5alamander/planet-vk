#pragma once


#include <planet/vk/forward.hpp>


namespace planet::vk::engine {


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

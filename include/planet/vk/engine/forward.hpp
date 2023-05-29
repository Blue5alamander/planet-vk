#pragma once


namespace planet::vk::engine {


    class app;
    class renderer;


    namespace pipeline {


        class mesh;
        class sprite;
        class textured;


    }


    namespace ui {


        template<typename T>
        struct per_frame;
        template<typename Pipeline = pipeline::textured>
        struct tx;

    }

}

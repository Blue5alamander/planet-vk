#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d {


    class renderer final {
      public:
        renderer(engine2d::app &);

        engine2d::app &app;
    };


}

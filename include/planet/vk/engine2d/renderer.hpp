#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d {


    class renderer final {
        vk::graphics_pipeline create_pipeline();

      public:
        renderer(engine2d::app &);

        engine2d::app &app;

        vk::swap_chain swapchain{app.device, app.window.extents()};
        vk::graphics_pipeline pipeline{create_pipeline()};
    };


}

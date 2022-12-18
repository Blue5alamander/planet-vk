#pragma once


#include <planet/vk/engine2d/app.hpp>


namespace planet::vk::engine2d {


    struct pos {
        float x, y;
    };
    struct colour {
        float r, g, b;
    };
    struct vertex {
        pos p;
        colour c;
    };

    constexpr std::array vertices{
            vertex{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};


    class renderer final {
        vk::graphics_pipeline create_pipeline();

      public:
        renderer(engine2d::app &);

        engine2d::app &app;

        vk::swap_chain swapchain{app.device, app.window.extents()};
        vk::graphics_pipeline pipeline{create_pipeline()};

        vk::command_pool command_pool{app.device, app.instance.surface};
        vk::command_buffers command_buffers{
                command_pool, swapchain.frame_buffers.size()};

        vk::buffer<vertex> vertex_buffer{app.device, vertices};

        planet::vk::semaphore img_avail_semaphore{app.device},
                render_finished_semaphore{app.device};
        planet::vk::fence fence{app.device};
    };


}

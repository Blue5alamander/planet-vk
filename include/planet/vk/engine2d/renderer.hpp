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
            vertex{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            vertex{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            vertex{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            vertex{{-0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}}};
    constexpr std::array<std::uint16_t, 6> indices{0, 1, 2, 2, 3, 0};


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

        vk::buffer<vertex> vertex_buffer{
                app.device, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        vk::buffer<std::uint16_t> index_buffer{
                app.device, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

        planet::vk::semaphore img_avail_semaphore{app.device},
                render_finished_semaphore{app.device};
        planet::vk::fence fence{app.device};
    };


}

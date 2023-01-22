#include <planet/vk/engine2d.hpp>


int main(int const argc, char const *argv[]) {
    planet::vk::engine2d::app app{argc, argv, "2d example"};
    planet::vk::engine2d::renderer renderer{app};

    constexpr std::array vertices{
            planet::vk::engine2d::vertex{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            planet::vk::engine2d::vertex{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            planet::vk::engine2d::vertex{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            planet::vk::engine2d::vertex{{-0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}}};
    constexpr std::array<std::uint16_t, 6> indices{0, 1, 2, 2, 3, 0};

    planet::vk::buffer<planet::vk::engine2d::vertex> vertex_buffer{
            app.device, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    planet::vk::buffer<std::uint16_t> index_buffer{
            app.device, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};


    for (bool done = false; not done;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { done = true; }
            if (event.type == SDL_KEYDOWN
                && event.key.keysym.sym == SDLK_ESCAPE) {
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_CLOSE
                && event.window.windowID == SDL_GetWindowID(app.window.get())) {
                done = true;
            }
        }

        auto &cmd_buf = renderer.start({0.f, 0.f, 0.f, 1.f});

        std::array buffers{vertex_buffer.get()};
        std::array offset{VkDeviceSize{}};

        vkCmdBindVertexBuffers(
                cmd_buf.get(), 0, buffers.size(), buffers.data(),
                offset.data());
        vkCmdBindIndexBuffer(
                cmd_buf.get(), index_buffer.get(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(
                cmd_buf.get(), static_cast<uint32_t>(indices.size()), 1, 0, 0,
                0);

        renderer.submit_and_present();
    }

    return 0;
}

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

        auto &cb = renderer.start({0.f, 0.f, 0.f, 1.f});

        renderer.draw_2dmesh(vertices, indices);
        renderer.draw_2dmesh(vertices, indices, {0.75f, 0.75f});

        renderer.submit_and_present();
    }

    return 0;
}

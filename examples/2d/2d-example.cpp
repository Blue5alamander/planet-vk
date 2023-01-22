#include <planet/vk/engine2d.hpp>


int main(int const argc, char const *argv[]) {
    planet::vk::engine2d::app app{argc, argv, "2d example"};
    planet::vk::engine2d::renderer renderer{app};

    bool done = false;
    while (not done) {
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

        std::uint32_t const image_index = renderer.start();

        renderer.submit_and_present();
    }

    return 0;
}

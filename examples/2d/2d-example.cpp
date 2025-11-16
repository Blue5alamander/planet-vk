#include <planet/vk/engine.hpp>


namespace {
    felspar::coro::task<int> render_loop(
            planet::vk::engine::app &app,
            planet::vk::engine::renderer &renderer) {
        planet::vk::engine::pipeline::mesh mesh{
                {.renderer = renderer,
                 .vertex_shader{"planet-vk-engine/mesh.world.vert.spirv"}}};

        constexpr std::array vertices{
                planet::vertex::coloured{
                        {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                planet::vertex::coloured{
                        {0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                planet::vertex::coloured{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                planet::vertex::coloured{
                        {-0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}}};
        constexpr std::array<std::uint32_t, 6> indices{0, 1, 2, 2, 3, 0};

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
                    && event.window.windowID
                            == SDL_GetWindowID(app.window.get())) {
                    done = true;
                }
            }

            co_await renderer.start(planet::colour::black);

            mesh.this_frame.draw(vertices, indices);
            mesh.this_frame.draw(vertices, indices, {0.75f, 0.75f, 0.0f});

            renderer.submit_and_present();
        }
        co_return 1;
    }
}


int main(int const argc, char const *argv[]) {
    planet::version const v{"2d-example", "planet/2d-example", "0.1", 1};
    felspar::posix::promise_to_never_use_select();
    felspar::io::poll_warden warden{};
    planet::sdl::init sdl{warden, v};
    return planet::vk::engine::app{argc, argv, sdl, v}.run(render_loop);
}

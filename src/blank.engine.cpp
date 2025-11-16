#include <planet/log.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/engine/blank.hpp>
#include <planet/vk/engine/renderer.hpp>


using namespace std::literals;


namespace {
    struct blanking {
        planet::vk::engine::app &app;
        planet::vk::engine::renderer &renderer;
        planet::colour colour;

        bool active = true;

        felspar::coro::task<void> loop() {
            while (active) {
                co_await renderer.start(colour);
                renderer.submit_and_present();
                co_await app.sdl.io.sleep(10ms);
            }
        }
    };
}


felspar::coro::task<void> planet::vk::engine::blank(
        planet::vk::engine::app &app,
        planet::vk::engine::renderer &renderer,
        planet::colour const &colour) {
    planet::log::debug("Starting GPU blank");
    blanking blank{app, renderer, colour};
    felspar::coro::eager<> loop;
    loop.post(blank, &blanking::loop);
    co_await renderer.full_render_cycle();
    blank.active = false;
    co_await std::move(loop).release();
    planet::log::debug("Completed GPU blank");
}

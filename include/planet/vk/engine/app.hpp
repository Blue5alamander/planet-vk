#pragma once


#include <planet/ui/baseplate.hpp>
#include <planet/vk-sdl.hpp>


namespace planet::vk::engine {


    class renderer;


    /// Maximum number of frames that we're willing to deal with at any given time
    constexpr std::size_t max_frames_in_flight = 3;


    /// ## An engine for 2d texture based interfaces
    class app final {
        std::unique_ptr<felspar::io::warden> warden;

      public:
        app(int argc, char const *argv[], char const *name);

        planet::asset_manager asset_manager;
        planet::sdl::init sdl;
        planet::sdl::ttf text{sdl};
        planet::vk::sdl::window window;
        planet::vk::extensions extensions{window};
        vk::instance instance;
        vk::device device{instance, extensions};

        planet::ui::baseplate<planet::vk::engine::renderer> baseplate;

        /// ### Run the provided UI function
        int run(felspar::coro::task<int> (*co_main)(app &, renderer &));
    };


}

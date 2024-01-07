#pragma once


#include <planet/ui/baseplate.hpp>
#include <planet/vk-sdl.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine {


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

        planet::ui::baseplate baseplate;


        /// ### Run the provided UI function
        int run(felspar::coro::task<int> (*co_main)(app &, renderer &));
    };


}

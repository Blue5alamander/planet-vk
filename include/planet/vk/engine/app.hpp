#pragma once


#include <planet/ui/baseplate.hpp>
#include <planet/vk-sdl.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine {


    /**
     * ## Application
     *
     * A single instance of this should be created in order to manage the window
     * and other instances needed to be able to run a Vulkan based application.
     */
    struct app final {
        app(int argc, char const *argv[], planet::sdl::init &, version const &);


        planet::asset_manager asset_manager;
        planet::sdl::init &sdl;
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

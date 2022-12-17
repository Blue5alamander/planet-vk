#pragma once


#include <planet/vk-sdl.hpp>


namespace planet::vk::engine2d {


    /// An engine for 2d texture based interfaces
    class app final {
        std::unique_ptr<felspar::io::warden> warden;

      public:
        app(int argc, char const *argv[], char const *name);

        planet::asset_manager asset_manager;
        planet::sdl::init sdl{*warden};
        planet::vk::sdl::window window;
        planet::vk::extensions extensions{window};
        vk::instance instance;
    };


}

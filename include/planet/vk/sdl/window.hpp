#pragma once


#include <planet/sdl/renderer.hpp>

#include <SDL.h>

#include <cstdint>


namespace planet::sdl {
    class init;
}


namespace planet::vk::sdl {


    class window final {
        planet::sdl::init &sdl;
        planet::sdl::handle<SDL_Window, SDL_DestroyWindow> pw;
        affine::extents2d size;

      public:
        window(planet::sdl::init &,
               const char *name,
               std::size_t width,
               std::size_t height);
        window(planet::sdl::init &, const char *name, std::uint32_t flags = {});

        SDL_Window *get() const noexcept { return pw.get(); }

        /// Current inner window size
        affine::extents2d const &extents() const noexcept { return size; }
        affine::rectangle2d rectangle() const noexcept {
            return {{0, 0}, size};
        }
        std::size_t zwidth() const noexcept { return size.zwidth(); }
        std::size_t zheight() const noexcept { return size.zheight(); }
        float width() const noexcept { return size.width; }
        float height() const noexcept { return size.height; }
    };


}

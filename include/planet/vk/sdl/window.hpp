#pragma once


#include <planet/sdl/renderer.hpp>

#include <SDL.h>

#include <cstdint>


namespace planet::sdl {
    class init;
}


namespace planet::vk::sdl {


    /// ## Vulkan Window
    class window final {
        planet::sdl::handle<SDL_Window, SDL_DestroyWindow> pw;
        affine::extents2d size;


      public:
        window(planet::sdl::init &,
               const char *name,
               std::size_t width,
               std::size_t height);
        window(planet::sdl::init &, const char *name, std::uint32_t flags = {});

        SDL_Window *get() const noexcept { return pw.get(); }


        /// ### Refresh the window dimensions
        affine::extents2d const &refresh_window_dimensions();


        /// ### Cached inner window size
        affine::extents2d const &extents() const noexcept { return size; }
        affine::rectangle2d rectangle() const noexcept {
            return {{0, 0}, size};
        }
        std::size_t uzwidth() const noexcept { return size.uzwidth(); }
        std::size_t uzheight() const noexcept { return size.uzheight(); }
        float width() const noexcept { return size.width; }
        float height() const noexcept { return size.height; }

        using constrained_type = ui::constrained2d<float>;
        constrained_type constraints() const noexcept {
            return {{size.width, 0, size.width}, {size.height, 0, size.height}};
        }
    };


}

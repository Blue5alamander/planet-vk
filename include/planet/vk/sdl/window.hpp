#pragma once


#include <planet/affine2d.hpp>
#include <planet/sdl/forward.hpp>
#include <planet/sdl/handle.hpp>
#include <planet/sdl/sdl.hpp>
#include <planet/ui/constrained.hpp>


namespace planet::vk::sdl {


    /// ## Vulkan Window
    class window final {
        planet::sdl::handle<SDL_Window, SDL_DestroyWindow> pw;
        affine::extents2d desktop_size, window_size;


      public:
        window(planet::sdl::init &,
               const char *name,
               std::size_t width,
               std::size_t height);
        window(planet::sdl::init &, const char *name, std::uint32_t flags = {});


        SDL_Window *get() const noexcept { return pw.get(); }


        /// ### Refresh the window dimensions
        affine::extents2d const &refresh_window_dimensions();


        /// ### Desktop size of the display this window is on
        affine::extents2d const &display_extents() const noexcept {
            return desktop_size;
        }
        /**
         * The full resolution of the monitor the window occupied as of the last
         * `refresh_window_dimensions()`, independent of whether the window is
         * full-screen or smaller. Use this in preference to `extents()` when a
         * size must stay constant as the window is resized — for example font
         * sizes that should render at a fixed physical size in either mode.
         */


        /// ### Cached inner window size
        affine::extents2d const &extents() const noexcept {
            return window_size;
        }
        affine::rectangle2d rectangle() const noexcept {
            return {{0, 0}, window_size};
        }
        std::size_t uzwidth() const noexcept { return window_size.uzwidth(); }
        std::size_t uzheight() const noexcept { return window_size.uzheight(); }
        float width() const noexcept { return window_size.width; }
        float height() const noexcept { return window_size.height; }

        using constrained_type = ui::constrained2d<float>;
        constrained_type constraints() const noexcept {
            return {{window_size.width, 0, window_size.width},
                    {window_size.height, 0, window_size.height}};
        }
    };


}

#pragma once


#include <planet/affine2d.hpp>
#include <planet/sdl/forward.hpp>
#include <planet/sdl/handle.hpp>
#include <planet/sdl/init.hpp>
#include <planet/sdl/pixel_density.hpp>
#include <planet/sdl/sdl.hpp>
#include <planet/ui/constrained.hpp>


namespace planet::vk::sdl {


    /// ## Vulkan Window
    class window final {
        planet::sdl::handle<SDL_Window, SDL_DestroyWindow> pw;
        affine::extents2d desktop_size, window_size;


        /// ### Place a windowed window where the configuration asks for
        void place_windowed(planet::sdl::configuration const &);
        /// ### Size and position a borderless window over the whole display
        void cover_display();


      public:
        window(planet::sdl::init &,
               const char *name,
               std::size_t width,
               std::size_t height);
        /// Use when there is no saved configuration

        window(planet::sdl::init &, const char *name);
        /**
         * Create the window honouring the `window_mode` and geometry in
         * `init`'s `configuration`.
         */


        SDL_Window *get() const noexcept { return pw.get(); }


        /// ### Refresh the window dimensions
        affine::extents2d const &refresh_window_dimensions();


        /// ### Change the display mode of the live window
        void change_display_mode(
                planet::sdl::configuration &, planet::sdl::window_mode);
        /**
         * Switches the window between the windowed and full-screen modes and
         * records the new mode in the configuration, so a game can offer the
         * choice as a setting that takes effect as it is made rather than on
         * the next launch.
         *
         * The windowed geometry survives the change: the size and position of a
         * windowed window are stored into the configuration (through
         * `store_geometry`) before the mode is changed, and are restored on the
         * way back to a windowed mode. Nothing here writes `window_extents` for
         * a full-screen window, so a spell in full screen never costs the
         * player the window size they had.
         *
         * The windowing system may apply the change asynchronously, so the
         * window can still report its old size when this returns. The resize
         * that follows drives the swap chain rebuild, exactly as a resize the
         * player makes by dragging the window does.
         */


        /// ### Store the current window geometry into a configuration
        void store_geometry(planet::sdl::configuration &) const noexcept;
        /**
         * When the window is in one of the windowed modes, copy its current
         * position and size into the configuration's `window_position` /
         * `window_extents` so they can be persisted and restored next launch.
         * The configuration position and size is only saved for a windowed
         * mode, so a saved windowed geometry survives a session spent
         * full-screen.
         */


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


        /// ### Display pixel density
        affine::extents2d pixel_density() const noexcept {
            return planet::sdl::pixel_density(pw.get());
        }
        /**
         * The number of drawable pixels per logical point on the display the
         * window is on. This is `> 1` on HiDPI/Retina displays and `1`
         * otherwise. `display_extents()` is reported in the display's logical
         * points, so multiply by this when a size must map onto drawable pixels
         * -- for example font sizes, which would otherwise render at half their
         * intended physical size on a `2` density Retina display.
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

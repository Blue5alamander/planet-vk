#include <planet/log.hpp>
#include <planet/sdl/init.hpp>
#include <planet/vk/buffer.hpp>
#include <planet/vk/commands.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/memory.hpp>
#include <planet/vk/sdl/texture.hpp>
#include <planet/vk/sdl/window.hpp>

#include <felspar/exceptions/logic_error.hpp>
#include <felspar/exceptions/runtime_error.hpp>

#include <SDL3/SDL_vulkan.h>


using namespace std::literals;


/// ## `planet::vk::extensions`


planet::vk::extensions::extensions(vk::sdl::window &w) : extensions{} {
    /**
     * SDL3 collapses the SDL2 two-call query (count, then names) into a single
     * call returning an SDL-owned array of extension name pointers — NULL on
     * failure — and drops the window argument. Append those pointers to
     * `vulkan_extensions`; they remain owned by SDL for the life of the
     * subsystem.
     */
    (void)w;
    Uint32 count{};
    char const *const *const names = SDL_Vulkan_GetInstanceExtensions(&count);
    if (not names) { throw felspar::stdexcept::runtime_error{SDL_GetError()}; }
    vulkan_extensions.insert(vulkan_extensions.end(), names, names + count);
}


/// ## `planet::vk::sdl::window`


/**
 * `SDL_WINDOW_HIGH_PIXEL_DENSITY` asks SDL for a drawable at the display's true
 * pixel size rather than its logical point size. On a HiDPI/Retina display --
 * e.g. a 4K panel driven in a "looks like 1080p" scaled mode -- this makes the
 * swapchain render at full native resolution instead of being drawn at the
 * point size and upscaled (fuzzily) by the compositor.
 * `refresh_window_dimensions` already reports the pixel size via
 * `SDL_GetWindowSizeInPixels`, so the swapchain sizing follows automatically.
 * Mouse events stay in logical points, so `planet::sdl::event_loop` scales them
 * by the window's pixel density to match.
 */
planet::vk::sdl::window::window(
        planet::sdl::init &,
        const char *const name,
        std::size_t const width,
        std::size_t const height)
: pw{SDL_CreateWindow(
          name,
          width,
          height,
          SDL_WINDOW_VULKAN bitor SDL_WINDOW_HIGH_PIXEL_DENSITY)},
  desktop_size{},
  window_size{float(width), float(height)} {
    if (not pw.get()) {
        throw felspar::stdexcept::runtime_error{"SDL_CreateWindow failed"};
    }
    refresh_window_dimensions();
}


namespace {
    /// #### SDL window flags for a `window_mode`
    /**
     * `full_screen_windowed` uses a plain `SDL_WINDOW_FULLSCREEN` window whose
     * fullscreen mode is left unset (`NULL`) — the SDL3 borderless
     * fullscreen-desktop equivalent. `full_screen_borderless` is a borderless
     * window (not SDL-fullscreen) that is separately sized to the desktop.
     */
    std::uint32_t flags_for(planet::sdl::window_mode const mode) noexcept {
        switch (mode) {
        case planet::sdl::window_mode::windowed: return 0u;
        case planet::sdl::window_mode::full_screen_windowed:
            return SDL_WINDOW_FULLSCREEN;
        case planet::sdl::window_mode::full_screen_borderless:
            return SDL_WINDOW_BORDERLESS;
        }
        return 0u;
    }
}


planet::vk::sdl::window::window(planet::sdl::init &sdl, const char *const name)
// High-DPI drawable, as in the (width, height) constructor above.
: pw{SDL_CreateWindow(
          name,
          static_cast<int>(sdl.config.window_extents.uzwidth()),
          static_cast<int>(sdl.config.window_extents.uzheight()),
          flags_for(sdl.config.window_display_mode) bitor SDL_WINDOW_VULKAN
                  bitor SDL_WINDOW_HIGH_PIXEL_DENSITY)},
  desktop_size{},
  window_size{sdl.config.window_extents} {
    if (not pw.get()) {
        throw felspar::stdexcept::runtime_error{"SDL_CreateWindow failed"};
    }
    switch (sdl.config.window_display_mode) {
    case planet::sdl::window_mode::windowed:
        /**
         * SDL3's `SDL_CreateWindow` no longer takes an initial position, so
         * apply the saved position, or centre the window when none is saved.
         */
        if (sdl.config.window_position) {
            SDL_SetWindowPosition(
                    pw.get(), static_cast<int>(sdl.config.window_position->x()),
                    static_cast<int>(sdl.config.window_position->y()));
        } else {
            SDL_SetWindowPosition(
                    pw.get(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
        break;
    case planet::sdl::window_mode::full_screen_borderless: {
        /**
         * A borderless window is not SDL-fullscreen, so size and position it to
         * cover the whole desktop of the display it opened on.
         */
        SDL_DisplayID const found = SDL_GetDisplayForWindow(pw.get());
        SDL_DisplayID const display =
                found == 0 ? SDL_GetPrimaryDisplay() : found;
        SDL_Rect bounds{};
        if (SDL_GetDisplayBounds(display, &bounds)) {
            SDL_SetWindowPosition(pw.get(), bounds.x, bounds.y);
            SDL_SetWindowSize(pw.get(), bounds.w, bounds.h);
        }
        break;
    }
    case planet::sdl::window_mode::full_screen_windowed:
        /**
         * `SDL_WINDOW_FULLSCREEN` with the fullscreen mode left `NULL` already
         * covers the whole display, so there is nothing more to do.
         */
        break;
    }
    refresh_window_dimensions();
    planet::log::info("Window created", window_size);
}


void planet::vk::sdl::window::store_geometry(
        planet::sdl::configuration &config) const noexcept {
    /**
     * Only a windowed window has a position and size worth remembering; the
     * full-screen modes cover the display, so leave the saved windowed geometry
     * alone for when the player switches back. Position and size are read in
     * logical points -- the same units
     * `SDL_CreateWindow`/`SDL_SetWindowPosition` take -- so they round-trip
     * through the configuration.
     */
    if (config.window_display_mode == planet::sdl::window_mode::windowed) {
        int x{}, y{}, w{}, h{};
        SDL_GetWindowPosition(pw.get(), &x, &y);
        SDL_GetWindowSize(pw.get(), &w, &h);
        config.window_position = affine::point2d{float(x), float(y)};
        config.window_extents = affine::extents2d{float(w), float(h)};
    }
}


auto planet::vk::sdl::window::refresh_window_dimensions()
        -> affine::extents2d const & {
    int ww{}, wh{};
    SDL_GetWindowSizeInPixels(pw.get(), &ww, &wh);
    window_size = {float(ww), float(wh)};

    /**
     * SDL3 replaces the index-based `SDL_GetWindowDisplayIndex` /
     * `SDL_GetDesktopDisplayMode(index, &mode)` pair with ID-based calls:
     * `SDL_GetDisplayForWindow` returns an `SDL_DisplayID` (0 on failure) and
     * `SDL_GetDesktopDisplayMode` returns a pointer to the internally-owned
     * mode rather than filling a caller-provided struct. Because the `0`
     * failure marker is not a valid display ID, fall back to the primary
     * display. The returned pointer may still be `NULL` on failure, so guard
     * the dereference.
     */
    SDL_DisplayID const found = SDL_GetDisplayForWindow(pw.get());
    SDL_DisplayMode const *const mode = SDL_GetDesktopDisplayMode(
            found == 0 ? SDL_GetPrimaryDisplay() : found);
    desktop_size = {mode ? float(mode->w) : 0.0f, mode ? float(mode->h) : 0.0f};

    return window_size;
}


/// ## Font and texture writing


planet::vk::texture planet::vk::sdl::create_texture_without_mip_levels(
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface,
        create_parameters const cp_args) {
    return create_texture_without_mip_levels(
            image_allocator.device->staging_memory, image_allocator, cp,
            surface, cp_args);
}
planet::vk::texture planet::vk::sdl::create_texture_without_mip_levels(
        device_memory_allocator &staging_allocator,
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface,
        create_parameters const cp_args) {
    if (surface.get()->format != SDL_PIXELFORMAT_ARGB8888) {
        throw felspar::stdexcept::logic_error{
                "Unexpected pixel format: "
                + std::string{SDL_GetPixelFormatName(surface.get()->format)}};
    }

    std::size_t const byte_count = surface.height() * surface.width() * 4;

    buffer<std::byte> staging{
            staging_allocator.device().staging_memory, byte_count,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    auto mapped{staging.map()};
    std::byte *const dest_base = mapped.get();
    std::byte const *const src_base =
            reinterpret_cast<std::byte const *>(surface.get()->pixels);
    std::size_t const line_bytes = surface.width() * 4;
    for (std::size_t line{}; line < surface.height(); ++line) {
        std::size_t const dest_offset = line * line_bytes;
        std::size_t const src_offset = line * surface.get()->pitch;
        std::memcpy(dest_base + dest_offset, src_base + src_offset, line_bytes);
    }

    return planet::vk::texture::create_without_mip_levels_from(
            {.allocator = image_allocator,
             .command_pool = cp,
             .buffer = staging,
             .width = static_cast<std::uint32_t>(surface.width()),
             .height = static_cast<std::uint32_t>(surface.height()),
             .scale = surface.fit,
             .address_mode = cp_args.address_mode,
             .filter = cp_args.filter,
             .border_color = cp_args.border_color});
}


planet::vk::texture planet::vk::sdl::create_texture_with_mip_levels(
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface,
        create_parameters const cp_args) {
    return create_texture_with_mip_levels(
            image_allocator.device->staging_memory, image_allocator, cp,
            surface, cp_args);
}
planet::vk::texture planet::vk::sdl::create_texture_with_mip_levels(
        device_memory_allocator &staging_allocator,
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface,
        create_parameters const cp_args) {
    if (surface.get()->format != SDL_PIXELFORMAT_ARGB8888) {
        throw felspar::stdexcept::logic_error{
                "Unexpected pixel format: "
                + std::string{SDL_GetPixelFormatName(surface.get()->format)}};
    }

    std::size_t const byte_count = surface.height() * surface.width() * 4;

    buffer<std::byte> staging{
            staging_allocator.device().staging_memory, byte_count,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    auto mapped{staging.map()};
    std::byte *const dest_base = mapped.get();
    std::byte const *const src_base =
            reinterpret_cast<std::byte const *>(surface.get()->pixels);
    std::size_t const line_bytes = surface.width() * 4;
    for (std::size_t line{}; line < surface.height(); ++line) {
        std::size_t const dest_offset = line * line_bytes;
        std::size_t const src_offset = line * surface.get()->pitch;
        std::memcpy(dest_base + dest_offset, src_base + src_offset, line_bytes);
    }

    return planet::vk::texture::create_with_mip_levels_from(
            {.allocator = image_allocator,
             .command_pool = cp,
             .buffer = staging,
             .width = static_cast<std::uint32_t>(surface.width()),
             .height = static_cast<std::uint32_t>(surface.height()),
             .scale = surface.fit,
             .address_mode = cp_args.address_mode,
             .filter = cp_args.filter,
             .border_color = cp_args.border_color});
}

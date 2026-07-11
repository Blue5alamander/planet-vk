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

#include <cstring>


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


planet::vk::sdl::window::window(
        planet::sdl::init &,
        const char *const name,
        std::size_t const width,
        std::size_t const height)
: pw{SDL_CreateWindow(name, width, height, SDL_WINDOW_VULKAN)},
  desktop_size{},
  window_size{float(width), float(height)} {
    if (not pw.get()) {
        throw felspar::stdexcept::runtime_error{"SDL_CreateWindow failed"};
    }
    refresh_window_dimensions();
}


planet::vk::sdl::window::window(
        planet::sdl::init &, const char *const name, std::uint32_t flags)
: pw{SDL_CreateWindow(name, 640, 480, flags | SDL_WINDOW_VULKAN)},
  desktop_size{},
  window_size{640, 480} {
    if (not pw.get()) {
        throw felspar::stdexcept::runtime_error{"SDL_CreateWindow failed"};
    }
    /**
     * SDL3's `SDL_CreateWindow` no longer takes an initial position. Re-apply
     * the centred behaviour with `SDL_SetWindowPosition`, but only when the
     * window is not full-screen — a full-screen window covers the whole display
     * regardless, so centring it is a no-op.
     */
    if (not(flags bitand SDL_WINDOW_FULLSCREEN)) {
        SDL_SetWindowPosition(
                pw.get(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    refresh_window_dimensions();
    planet::log::info("Window created", window_size);
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
        throw felspar::stdexcept::logic_error{"Unexpected pixel format"};
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
        throw felspar::stdexcept::logic_error{"Unexpected pixel format"};
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

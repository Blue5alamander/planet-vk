#include <planet/log.hpp>
#include <planet/sdl/init.hpp>
#include <planet/vk/buffer.hpp>
#include <planet/vk/commands.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/memory.hpp>
#include <planet/vk/sdl/texture.hpp>
#include <planet/vk/sdl/window.hpp>

#include <felspar/exceptions.hpp>

#include <SDL_vulkan.h>

#include <cstring>


using namespace std::literals;


/// ## `planet::vk::extensions`


planet::vk::extensions::extensions(vk::sdl::window &w) : extensions{} {
    unsigned int count;
    if (not SDL_Vulkan_GetInstanceExtensions(w.get(), &count, nullptr)) {
        throw felspar::stdexcept::runtime_error{SDL_GetError()};
    }
    auto const existing_extension_count = vulkan_extensions.size();
    vulkan_extensions.resize(existing_extension_count + count);
    if (not SDL_Vulkan_GetInstanceExtensions(
                w.get(), &count,
                vulkan_extensions.data() + existing_extension_count)) {
        throw felspar::stdexcept::runtime_error{SDL_GetError()};
    }
}


/// ## `planet::vk::sdl::window`


planet::vk::sdl::window::window(
        planet::sdl::init &,
        const char *const name,
        std::size_t const width,
        std::size_t const height)
: pw{SDL_CreateWindow(
          name,
          SDL_WINDOWPOS_UNDEFINED,
          SDL_WINDOWPOS_UNDEFINED,
          width,
          height,
          SDL_WINDOW_VULKAN)},
  size{float(width), float(height)} {}


planet::vk::sdl::window::window(
        planet::sdl::init &, const char *const name, std::uint32_t flags)
: pw{SDL_CreateWindow(
          name,
          SDL_WINDOWPOS_CENTERED,
          SDL_WINDOWPOS_CENTERED,
          640,
          480,
          flags | SDL_WINDOW_VULKAN)},
  size{640, 480} {
    if (not pw.get()) {
        throw felspar::stdexcept::runtime_error{"SDL_CreateWindow failed"};
    }
    refresh_window_dimensions();
    planet::log::info("Window created", size);
}


auto planet::vk::sdl::window::refresh_window_dimensions() -> affine::extents2d const & {
    int ww{}, wh{};
    SDL_Vulkan_GetDrawableSize(pw.get(), &ww, &wh);
    return size = {float(ww), float(wh)};
}


/// ## Font and texture writing


planet::vk::texture planet::vk::sdl::create_texture_without_mip_levels(
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface) {
    return create_texture_without_mip_levels(
            image_allocator.device->staging_memory, image_allocator, cp,
            surface);
}
planet::vk::texture planet::vk::sdl::create_texture_without_mip_levels(
        device_memory_allocator &staging_allocator,
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface) {
    if (surface.get()->format->format != SDL_PIXELFORMAT_ARGB8888) {
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
             .scale = surface.fit});
}


planet::vk::texture planet::vk::sdl::create_texture_with_mip_levels(
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface) {
    return create_texture_with_mip_levels(
            image_allocator.device->staging_memory, image_allocator, cp,
            surface);
}
planet::vk::texture planet::vk::sdl::create_texture_with_mip_levels(
        device_memory_allocator &staging_allocator,
        device_memory_allocator &image_allocator,
        command_pool &cp,
        planet::sdl::surface const &surface) {
    if (surface.get()->format->format != SDL_PIXELFORMAT_ARGB8888) {
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
             .scale = surface.fit});
}

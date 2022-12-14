#include <planet/sdl/init.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/sdl/window.hpp>

#include <felspar/exceptions.hpp>

#include <SDL_vulkan.h>

#include <iostream>


using namespace std::literals;


/**
 * ## planet::vk::sdl::window
 */


planet::vk::sdl::window::window(
        planet::sdl::init &s,
        const char *const name,
        std::size_t const width,
        std::size_t const height)
: sdl{s},
  pw{SDL_CreateWindow(
          name,
          SDL_WINDOWPOS_UNDEFINED,
          SDL_WINDOWPOS_UNDEFINED,
          width,
          height,
          SDL_WINDOW_VULKAN)},
  size{float(width), float(height)} {}


planet::vk::sdl::window::window(
        planet::sdl::init &s, const char *const name, std::uint32_t flags)
: sdl{s},
  pw{SDL_CreateWindow(
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
    std::cout << "Window created\n";
    int ww{}, wh{};
    SDL_Vulkan_GetDrawableSize(pw.get(), &ww, &wh);
    size = {float(ww), float(wh)};
}


/**
 * ## planet::vk::extensions
 */


planet::vk::extensions::extensions(sdl::window &w) {
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

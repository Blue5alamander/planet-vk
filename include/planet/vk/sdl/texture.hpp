#pragma once


#include <planet/colour.hpp>
#include <planet/sdl/ttf.hpp>
#include <planet/vk/forward.hpp>
#include <planet/vk/texture.hpp>


namespace planet::vk::sdl {


    /// ## Convert a `vk::colour` to a `SDL_Color`
    inline SDL_Color to_sdl_color(colour const &c) noexcept {
        auto const convert = [](float const v) {
            return static_cast<std::uint8_t>(v * 255.0f);
        };
        return {convert(c.r), convert(c.g), convert(c.b), convert(c.a)};
    }


    /// ## Create a Vulkan texture from an SDL surface
    texture create_texture_with_mip_levels(
            device_memory_allocator &image,
            command_pool &,
            planet::sdl::surface const &);
    texture create_texture_with_mip_levels(
            device_memory_allocator &staging,
            device_memory_allocator &image,
            command_pool &,
            planet::sdl::surface const &);
    texture create_texture_without_mip_levels(
            device_memory_allocator &staging,
            device_memory_allocator &image,
            command_pool &,
            planet::sdl::surface const &);
    texture create_texture_without_mip_levels(
            device_memory_allocator &image,
            command_pool &,
            planet::sdl::surface const &);


    /// ##  Render text and turn it into a Vulkan texture
    inline texture
            render(device_memory_allocator &staging,
                   device_memory_allocator &image,
                   command_pool &cp,
                   planet::sdl::font const &f,
                   char const *t,
                   colour const &col = colour::white) {
        return create_texture_without_mip_levels(
                staging, image, cp, f.render(t, to_sdl_color(col)));
    }
    inline texture
            render(device_memory_allocator &image,
                   command_pool &cp,
                   planet::sdl::font const &f,
                   char const *t,
                   colour const &col = colour::white) {
        return create_texture_without_mip_levels(
                image, cp, f.render(t, to_sdl_color(col)));
    }


}

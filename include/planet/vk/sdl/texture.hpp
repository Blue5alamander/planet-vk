#pragma once


#include <planet/sdl/ttf.hpp>
#include <planet/vk/texture.hpp>


namespace planet::vk {
    class command_pool;
    class device_memory_allocator;
}


namespace planet::vk::sdl {


    /// ## Create a Vulkan texture from an SDL surface
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

    /// ##  Render text and turn it into a Vulkan texture
    inline texture
            render(device_memory_allocator &staging,
                   device_memory_allocator &image,
                   command_pool &cp,
                   planet::sdl::font const &f,
                   char const *t) {
        return create_texture_without_mip_levels(
                staging, image, cp, f.render(t));
    }


}

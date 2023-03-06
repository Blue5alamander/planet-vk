#pragma once


#include <planet/sdl/ttf.hpp>
#include <planet/vk/texture.hpp>


namespace planet::vk {
    class command_pool;
    class device_memory_allocator;
}


namespace planet::vk::sdl {


    /// Render the requested text and turn it into a Vulkan texture suitable for
    /// display
    texture
            render(device_memory_allocator &,
                   command_pool &,
                   planet::sdl::font const &,
                   char const *);


}

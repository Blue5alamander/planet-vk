#pragma once


#include <vulkan/vulkan.h>

#include <vector>


namespace planet::vk::sdl {
    class window;
}


namespace planet::vk {


    /// ## Vulkan extensions and validation layers
    struct extensions final {
        std::vector<char const *> vulkan_extensions,
                device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME},
                validation_layers;


        extensions();
        extensions(vk::sdl::window &);


        bool has_validation() const { return not validation_layers.empty(); }


        static std::span<VkLayerProperties const> supported_validation_layers();
    };


}

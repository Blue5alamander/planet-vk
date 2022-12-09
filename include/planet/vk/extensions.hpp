#pragma once


#include <vulkan/vulkan.h>

#include <vector>


namespace planet::vk {


    /// Store the Vulkan extensions
    struct extensions {
        std::vector<char const *> vulkan_extensions,
                device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME},
                validation_layers;

        bool has_validation() const { return not validation_layers.empty(); }
    };


}

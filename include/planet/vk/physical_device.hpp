#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class instance;


    class physical_device final {
        VkPhysicalDevice handle;

      public:
        physical_device(VkPhysicalDevice);

        VkPhysicalDevice get() const noexcept { return handle; }

        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        VkPhysicalDeviceMemoryProperties memory_properties = {};
        std::vector<VkQueueFamilyProperties> queue_family_properties;
    };


}

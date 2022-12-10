#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class instance;


    class physical_device final {
        VkPhysicalDevice handle;
        std::optional<std::uint32_t> graphics, present;

      public:
        physical_device(VkPhysicalDevice, VkSurfaceKHR);

        VkPhysicalDevice get() const noexcept { return handle; }

        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        VkPhysicalDeviceMemoryProperties memory_properties = {};
        std::vector<VkQueueFamilyProperties> queue_family_properties;

        bool has_queue_families() const noexcept {
            return graphics.has_value() and present.has_value();
        }

        std::uint32_t graphics_queue_index() const noexcept {
            return *graphics;
        }
        std::uint32_t presentation_queue_index() const noexcept {
            return *present;
        }
    };


}

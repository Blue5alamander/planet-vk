#pragma once


#include <planet/vk/forward.hpp>
#include <planet/vk/helpers.hpp>


namespace planet::vk {


    /// ## Vulkan physical device
    class physical_device final {
        VkPhysicalDevice handle;

      public:
        physical_device(VkPhysicalDevice, VkSurfaceKHR);

        VkPhysicalDevice get() const noexcept { return handle; }

        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        VkPhysicalDeviceMemoryProperties memory_properties = {};

        VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;

        /// ### API wrappers

        /// #### `vkGetPhysicalDeviceFormatProperties`
        VkFormatProperties format_properties(VkFormat) const;
    };


}

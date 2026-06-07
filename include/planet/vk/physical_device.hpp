#pragma once


#include <planet/vk/forward.hpp>
#include <planet/vk/helpers.hpp>

#include <cstdint>
#include <string>


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

        std::vector<VkExtensionProperties> extensions;


        /// ### API wrappers

        /// #### `vkGetPhysicalDeviceFormatProperties`
        VkFormatProperties format_properties(VkFormat) const;


        /// ### Human-readable description of a memory type index
        /**
         * Decodes a `memory_type_index` into its `VK_MEMORY_PROPERTY_*` flags
         * and heap, e.g. `2:DEVICE_LOCAL|HOST_VISIBLE@heap0`. The leading index
         * is kept because distinct types can share identical flags, so it keeps
         * the description unique within this GPU.
         */
        std::string memory_type_name(std::uint32_t memory_type_index) const;
    };


}

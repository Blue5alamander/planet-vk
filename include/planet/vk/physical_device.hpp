#pragma once


#include <planet/vk/forward.hpp>
#include <planet/vk/helpers.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>


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


    /// ## The `VK_KHR_portability_subset` device extension name
    /**
     * `VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME` is only defined when
     * `VK_ENABLE_BETA_EXTENSIONS` is set (it lives in `vulkan_beta.h`, which
     * would also pull in every other provisional extension), so the one name we
     * need is spelled out here instead.
     */
    inline constexpr char const *portability_subset_extension_name =
            "VK_KHR_portability_subset";


    /// ## Device extensions to enable, given what the device advertises
    /**
     * Returns `requested` plus any extension that the Vulkan spec *requires* be
     * enabled at device creation when the physical device advertises it. In
     * particular `VK_KHR_portability_subset` must be enabled whenever it is
     * present -- portability ICDs such as MoltenVK (macOS) report it, whereas
     * conformant Linux/Windows drivers never do, so `requested` is returned
     * unchanged there.
     *
     * `available` is the device's advertised extensions (see
     * `physical_device::extensions`).
     */
    std::vector<char const *> required_device_extensions(
            std::span<VkExtensionProperties const> available,
            std::vector<char const *> requested);


}

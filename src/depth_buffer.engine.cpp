#include <planet/vk/engine/depth_buffer.hpp>
#include <planet/vk/physical_device.hpp>


VkFormat planet::vk::engine::depth_buffer::find_supported_format(
        vk::physical_device const &device,
        std::span<VkFormat> candidates,
        VkImageTiling const tiling,
        VkFormatFeatureFlags const features) {
    for (auto format : candidates) {
        auto const properties = device.format_properties(format);
        if (tiling == VK_IMAGE_TILING_LINEAR
            and (properties.linearTilingFeatures bitand features) == features) {
            return format;
        } else if (
                tiling == VK_IMAGE_TILING_OPTIMAL
                and (properties.optimalTilingFeatures bitand features)
                        == features) {
            return format;
        }
    }
    throw felspar::stdexcept::runtime_error{
            "No suitable depth buffer format found"};
}

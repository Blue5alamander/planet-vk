#include <planet/vk/engine/depth_buffer.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/physical_device.hpp>
#include <planet/vk/swap_chain.hpp>


planet::vk::engine::depth_buffer::depth_buffer(vk::swap_chain &swap_chain)
: image{swap_chain.device->startup_memory,
        swap_chain.extents.width,
        swap_chain.extents.height,
        1,
        swap_chain.device->instance.gpu().msaa_samples,
        default_format(swap_chain.device->instance.gpu()),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
  image_view{image, VK_IMAGE_ASPECT_DEPTH_BIT} {}


VkFormat planet::vk::engine::depth_buffer::find_supported_format(
        physical_device const &device,
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


VkFormat planet::vk::engine::depth_buffer::default_format(
        physical_device const &device) {
    return planet::vk::engine::depth_buffer::find_supported_format(
            device,
            std::array{
                    VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                    VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

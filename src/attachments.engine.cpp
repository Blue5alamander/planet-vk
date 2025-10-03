#include <planet/vk/engine/colour_attachment.hpp>
#include <planet/vk/engine/depth_buffer.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/physical_device.hpp>
#include <planet/vk/swap_chain.hpp>


/// ## `planet::vk::engine::colour_attachment`


planet::vk::engine::colour_attachment::colour_attachment(parameters p)
: image{p.allocator, p.extents.width, p.extents.height, 1,
        /**
         * TODO This is a hack for working out the correct sampling value. There
         * should be something more sensible for this
         */
        p.sample_count, p.format, VK_IMAGE_TILING_OPTIMAL,
        static_cast<VkImageUsageFlagBits>(
                p.usage_flags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
  image_view{image, VK_IMAGE_ASPECT_COLOR_BIT} {}


VkAttachmentDescription
        planet::vk::engine::colour_attachment::attachment_description(
                VkSampleCountFlagBits const samples,
                VkAttachmentLoadOp const clear) const {
    VkAttachmentDescription ca{};
    ca.format = image.format;
    ca.samples = samples;
    ca.loadOp = clear;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ca.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return ca;
}
VkAttachmentDescription
        planet::vk::engine::colour_attachment::attachment_description(
                physical_device const &device) const {
    return attachment_description(
            device.msaa_samples, VK_ATTACHMENT_LOAD_OP_CLEAR);
}


/// ## `planet::vk::engine::depth_buffer`


planet::vk::engine::depth_buffer::depth_buffer(
        device_memory_allocator &allocator, vk::swap_chain &swap_chain)
: image{allocator,
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


VkAttachmentDescription planet::vk::engine::depth_buffer::attachment_description(
        physical_device const &device) const {
    VkAttachmentDescription da{};
    da.format = image.format;
    da.samples = device.msaa_samples;
    da.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    da.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    da.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    da.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    da.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    da.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    return da;
}

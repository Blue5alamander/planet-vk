#include <planet/vk/engine/colour_attachment.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/swap_chain.hpp>


planet::vk::engine::colour_attachment::colour_attachment(
        vk::swap_chain &swap_chain)
: image{swap_chain.device->startup_memory,
        swap_chain.extents.width,
        swap_chain.extents.height,
        1,
        swap_chain.device->instance.gpu().msaa_samples,
        swap_chain.image_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
  image_view{image, VK_IMAGE_ASPECT_COLOR_BIT} {}


VkAttachmentDescription
        planet::vk::engine::colour_attachment::attachment_description(
                physical_device const &device) const {
    VkAttachmentDescription ca{};
    ca.format = image.format;
    ca.samples = device.msaa_samples;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ca.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return ca;
}

#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/swap_chain.hpp>


std::uint32_t planet::vk::swap_chain::create(VkExtent2D const wsize) {
    std::uint32_t image_count =
            device.instance.surface.capabilities.minImageCount + 1;
    if (device.instance.surface.capabilities.maxImageCount > 0
        and image_count > device.instance.surface.capabilities.maxImageCount) {
        image_count = device.instance.surface.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = device.instance.surface.get();

    info.minImageCount = image_count;
    info.imageFormat = device.instance.surface.best_format.format;
    info.imageColorSpace = device.instance.surface.best_format.colorSpace;
    info.imageExtent = wsize;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::array queues{
            device.instance.surface.graphics_queue_index(),
            device.instance.surface.presentation_queue_index()};

    if (device.instance.surface.graphics_queue_index()
        != device.instance.surface.presentation_queue_index()) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = queues.data();
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform = device.instance.surface.capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = device.instance.surface.best_present_mode;
    info.clipped = VK_TRUE;

    handle.create<vkCreateSwapchainKHR>(device.get(), info);

    return image_count;
}
std::uint32_t planet::vk::swap_chain::create(affine::extents2d const wsize) {
    return create(extents(device, wsize));
}


VkExtent2D planet::vk::swap_chain::extents(
        vk::device const &device, affine::extents2d const ex) {
    if (device.instance.surface.capabilities.currentExtent.width
        != std::numeric_limits<std::uint32_t>::max()) {
        return device.instance.surface.capabilities.currentExtent;
    } else {
        return VkExtent2D{
                std::clamp<std::uint32_t>(
                        ex.width,
                        device.instance.surface.capabilities.minImageExtent.width,
                        device.instance.surface.capabilities.maxImageExtent
                                .width),
                std::clamp<std::uint32_t>(
                        ex.height,
                        device.instance.surface.capabilities.minImageExtent
                                .height,
                        device.instance.surface.capabilities.maxImageExtent
                                .height)};
    }
}

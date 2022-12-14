#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/swap_chain.hpp>


/**
 * ## planet::vk::swap_chain
 */


std::uint32_t planet::vk::swap_chain::create(VkExtent2D const wsize) {
    frame_buffers.clear();
    image_views.clear();
    images.clear();

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
    info.imageFormat = image_format =
            device.instance.surface.best_format.format;
    info.imageColorSpace = device.instance.surface.best_format.colorSpace;
    info.imageExtent = extents = wsize;
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

    images = fetch_vector<vkGetSwapchainImagesKHR, VkImage>(
            device.get(), handle.get());

    for (auto const image : images) { image_views.emplace_back(*this, image); }

    return images.size();
}
std::uint32_t planet::vk::swap_chain::create(affine::extents2d const wsize) {
    return create(swap_chain::calculate_extents(device, wsize));
}


VkExtent2D planet::vk::swap_chain::calculate_extents(
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


/**
 * ## planet::vk::image_view
 */


planet::vk::image_view::image_view(swap_chain const &sc, VkImage image)
: device{sc.device} {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = sc.image_format;

    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    handle.create<vkCreateImageView>(sc.device.get(), info);
}

#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/swap_chain.hpp>


/// ## `planet::vk::swap_chain`


std::uint32_t planet::vk::swap_chain::create(VkExtent2D const wsize) {
    frame_buffers.clear();
    image_views.clear();
    images.clear();

    auto &surface = device().instance.surface;

    std::uint32_t image_count = surface.capabilities.minImageCount + 1;
    if (surface.capabilities.maxImageCount > 0
        and image_count > surface.capabilities.maxImageCount) {
        image_count = surface.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface.get();

    info.minImageCount = image_count;
    info.imageFormat = image_format = surface.best_format.format;
    info.imageColorSpace = surface.best_format.colorSpace;
    info.imageExtent = extents = wsize;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::array queues{
            surface.graphics_queue_index(), surface.presentation_queue_index()};

    if (surface.graphics_queue_index() != surface.presentation_queue_index()) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = queues.data();
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform = surface.capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = surface.best_present_mode;
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
    auto const &surface = device.instance.surface;
    if (surface.capabilities.currentExtent.width
        != std::numeric_limits<std::uint32_t>::max()) {
        return surface.capabilities.currentExtent;
    } else {
        return VkExtent2D{
                std::clamp<std::uint32_t>(
                        ex.width, surface.capabilities.minImageExtent.width,
                        surface.capabilities.maxImageExtent.width),
                std::clamp<std::uint32_t>(
                        ex.height, surface.capabilities.minImageExtent.height,
                        surface.capabilities.maxImageExtent.height)};
    }
}

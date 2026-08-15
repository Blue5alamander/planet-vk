#include <planet/log.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/swap_chain.hpp>

#include <algorithm>
#include <array>


/// ## `planet::vk::swap_chain`


namespace {


    /// ### Composite alpha mode the surface will actually accept
    /**
     * The frame is drawn fully opaque, so `OPAQUE` is what we want -- but it is
     * not universally offered. Android surfaces commonly support only
     * `INHERIT`, where desktop WSI supports `OPAQUE`, and passing a bit the
     * surface does not advertise is undefined behaviour: the validation layers
     * reject it while drivers may quietly carry on compositing the layer in
     * some other way. Pick the first supported mode in preference order rather
     * than assuming.
     */
    VkCompositeAlphaFlagBitsKHR composite_alpha_for(
            VkSurfaceCapabilitiesKHR const &capabilities) noexcept {
        static constexpr std::array preference{
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR};
        for (auto const bit : preference) {
            if (capabilities.supportedCompositeAlpha bitand bit) { return bit; }
        }
        /**
         * The spec requires at least one mode, so an empty mask means the
         * surface capabilities were never filled in.
         */
        planet::log::critical(
                "The surface advertises no supported composite alpha mode",
                capabilities.supportedCompositeAlpha);
        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }


}


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
    planet::log::info(
            "Swap chain creation minImageCount",
            surface.capabilities.minImageCount, "maxImageCount",
            surface.capabilities.maxImageCount, "settled on", image_count);


    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface.get();

    info.minImageCount = image_count;
    info.imageFormat = image_format = surface.best_format.format;
    info.imageColorSpace = surface.best_format.colorSpace;
    info.imageExtent = extents = wsize;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (transfer != transfer_source::not_requested) {
        if (surface.capabilities.supportedUsageFlags
            bitand VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            info.imageUsage =
                    info.imageUsage bitor VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            transfer = transfer_source::available;
        } else {
            transfer = transfer_source::not_available;
        }
    }

    std::array queues{
            surface.graphics_queue_family_index(),
            surface.presentation_queue_family_index()};

    if (surface.graphics_queue_family_index()
        != surface.presentation_queue_family_index()) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = queues.size();
        info.pQueueFamilyIndices = queues.data();
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform = surface.capabilities.currentTransform;
    info.compositeAlpha = composite_alpha_for(surface.capabilities);
    info.presentMode = surface.best_present_mode;
    info.clipped = VK_TRUE;

    handle.create<vkCreateSwapchainKHR>(device.get(), info);

    images = fetch_vector<vkGetSwapchainImagesKHR, VkImage>(
            device.get(), handle.get());

    for (auto const image : images) { image_views.emplace_back(*this, image); }

    planet::log::info(
            "Created swap chain with", images.size(), "images and extents",
            wsize.width, "x", wsize.height);

    return images.size();
}
std::uint32_t planet::vk::swap_chain::create(affine::extents2d const wsize) {
    return create(swap_chain::calculate_extents(device, wsize));
}


VkExtent2D planet::vk::swap_chain::calculate_extents(
        vk::device const &device, affine::extents2d const ex) {
    auto const &capabilities = device.instance.surface.capabilities;
    if (capabilities.currentExtent.width
        != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        return VkExtent2D{
                std::clamp<std::uint32_t>(
                        ex.width, capabilities.minImageExtent.width,
                        capabilities.maxImageExtent.width),
                std::clamp<std::uint32_t>(
                        ex.height, capabilities.minImageExtent.height,
                        capabilities.maxImageExtent.height)};
    }
}


VkAttachmentDescription planet::vk::swap_chain::attachment_description() const {
    VkAttachmentDescription ca{};
    ca.format = image_format;
    ca.samples = VK_SAMPLE_COUNT_1_BIT;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return ca;
}

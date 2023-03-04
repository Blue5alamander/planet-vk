#include <planet/vk/image.hpp>
#include <planet/vk/swap_chain.hpp>


/// ## `planet::vk::image`


planet::vk::image::image(
        device_memory_allocator &allocator,
        std::uint32_t const width,
        std::uint32_t const height,
        std::uint32_t const mip_levels,
        VkSampleCountFlagBits num_samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memory_flags) {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = 1;
    info.mipLevels = mip_levels;
    info.arrayLayers = 1;
    info.format = format;
    info.tiling = tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = usage;
    info.samples = num_samples;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    handle.create<vkCreateImage>(allocator.device.get(), info);

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(handle.owner(), handle.get(), &requirements);

    memory = allocator.allocate(requirements.size, requirements, memory_flags);
    memory.bind_image_memory(handle.get());
}


/// ## `planet::vk::image_view`


planet::vk::image_view::image_view(
        VkDevice const device,
        VkImage const image,
        VkFormat const format,
        VkImageAspectFlags const aspect_flags,
        std::uint32_t const mip_levels) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.aspectMask = aspect_flags;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mip_levels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    handle.create<vkCreateImageView>(device, info);
}


planet::vk::image_view::image_view(swap_chain const &sc, VkImage const image) {
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

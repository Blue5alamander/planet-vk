#include <planet/vk/buffer.hpp>
#include <planet/vk/commands.hpp>
#include <planet/vk/image.hpp>
#include <planet/vk/swap_chain.hpp>


/// ## `planet::vk::image`


planet::vk::image::image(
        device_memory_allocator &allocator,
        std::uint32_t const w,
        std::uint32_t const h,
        std::uint32_t const ml,
        VkSampleCountFlagBits const num_samples,
        VkFormat const f,
        VkImageTiling const tiling,
        VkImageUsageFlags const usage,
        VkMemoryPropertyFlags const memory_flags)
: width{w}, height{h}, mip_levels{ml}, format{f} {
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
    info.initialLayout = layout;
    info.usage = usage;
    info.samples = num_samples;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    handle.create<vkCreateImage>(allocator.device.get(), info);

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(handle.owner(), handle.get(), &requirements);

    memory = allocator.allocate(requirements, memory_flags);
    memory.bind_image_memory(handle.get());
}


void planet::vk::image::transition_layout(
        command_pool &cp,
        VkImageLayout const new_layout,
        std::uint32_t const mip_levels) {

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    VkAccessFlags source_access_flags;
    VkAccessFlags destination_access_flags;

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED
        && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        source_access_flags = VK_ACCESS_NONE;
        destination_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
            layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        source_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT;
        destination_access_flags = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw felspar::stdexcept::logic_error("Unsupported layout transition!");
    }

    planet::vk::command_buffer::single_use(cp)
            .pipeline_barrier(
                    source_stage, destination_stage,
                    std::array{transition(
                            {.new_layout = new_layout,
                             .source_access_mask = source_access_flags,
                             .destination_access_mask = destination_access_flags,
                             .mip_levels = mip_levels})})
            .end_and_submit();
}


void planet::vk::image::copy_from(
        command_pool &cp, vk::buffer<std::byte> const &buffer) {
    auto cb = planet::vk::command_buffer::single_use(cp);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
            cb.get(), buffer.get(), get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);

    cb.end_and_submit();
}


void planet::vk::image::generate_mip_maps(
        command_pool &cp, std::uint32_t const mip_levels) {
    if (not(cp.device().instance.gpu().format_properties(format).optimalTilingFeatures
            bitand VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw felspar::stdexcept::runtime_error{"Linear BLITing not supported"};
    }

    auto cb = planet::vk::command_buffer::single_use(cp);

    std::int32_t mip_width = width, next_mip_width = {};
    std::int32_t mip_height = height, next_mip_height = {};

    for (std::uint32_t i = 1; i < mip_levels; i++) {
        next_mip_width = mip_width > 1 ? mip_width / 2 : 1;
        next_mip_height = mip_height > 1 ? mip_height / 2 : 1;

        cb.pipeline_barrier(
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                std::array{transition(
                        {.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         .new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         .source_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                         .destination_access_mask = VK_ACCESS_TRANSFER_READ_BIT,
                         .base_mip_level = i - 1})});

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {next_mip_width, next_mip_height, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
                cb.get(), get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, get(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                VK_FILTER_LINEAR);

        cb.pipeline_barrier(
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                std::array{transition(
                        {.old_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         .new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         .source_access_mask = VK_ACCESS_TRANSFER_READ_BIT,
                         .destination_access_mask = VK_ACCESS_SHADER_READ_BIT,
                         .base_mip_level = i - 1})});

        mip_width = next_mip_width;
        mip_height = next_mip_height;
    }

    cb.pipeline_barrier(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            std::array{transition(
                    {.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     .new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     .source_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                     .destination_access_mask = VK_ACCESS_SHADER_READ_BIT,
                     .base_mip_level = mip_levels - 1})});

    cb.end_and_submit();
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

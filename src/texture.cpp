#include <planet/vk/buffer.hpp>
#include <planet/vk/commands.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/texture.hpp>

#include <cmath>


/// ## `planet::vk::sampler`


planet::vk::sampler::sampler(parameters p) : device{p.device} {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = p.address_mode;
    info.addressModeV = p.address_mode;
    info.addressModeW = p.address_mode;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy =
            device().instance.gpu().properties.limits.maxSamplerAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.minLod = 0.0f;
    info.maxLod = static_cast<float>(p.mip_levels);
    info.mipLodBias = 0.0f;

    handle.create<vkCreateSampler>(device.get(), info);
}


/// ## `planet::vk::texture`


namespace {
    void transition_layout(
            planet::vk::command_buffer &cb,
            planet::vk::image &image,
            VkImageLayout const new_layout,
            std::uint32_t const mip_levels) {

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;
        VkAccessFlags source_access_flags;
        VkAccessFlags destination_access_flags;

        if (image.layout == VK_IMAGE_LAYOUT_UNDEFINED
            && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            source_access_flags = VK_ACCESS_NONE;
            destination_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT;

            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (
                image.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            source_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT;
            destination_access_flags = VK_ACCESS_SHADER_READ_BIT;

            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw felspar::stdexcept::logic_error(
                    "Unsupported layout transition!");
        }

        cb.pipeline_barrier(
                source_stage, destination_stage,
                std::array{image.transition(
                        {.new_layout = new_layout,
                         .source_access_mask = source_access_flags,
                         .destination_access_mask = destination_access_flags,
                         .mip_levels = mip_levels})});
    }
    void copy_from(
            planet::vk::command_buffer &cb,
            planet::vk::image &image,
            planet::vk::buffer<std::byte> const &buffer) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {image.width, image.height, 1};

        vkCmdCopyBufferToImage(
                cb.get(), buffer.get(), image.get(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    void generate_mip_maps(
            planet::vk::command_buffer &cb,
            planet::vk::image &image,
            std::uint32_t const mip_levels) {
        if (not(cb.device()
                        .instance.gpu()
                        .format_properties(image.format)
                        .optimalTilingFeatures
                bitand VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw felspar::stdexcept::runtime_error{
                    "Linear BLITing not supported"};
        }

        std::int32_t mip_width = image.width, next_mip_width = {};
        std::int32_t mip_height = image.height, next_mip_height = {};

        for (std::uint32_t i = 1; i < mip_levels; i++) {
            next_mip_width = mip_width > 1 ? mip_width / 2 : 1;
            next_mip_height = mip_height > 1 ? mip_height / 2 : 1;

            cb.pipeline_barrier(
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    std::array{image.transition(
                            {.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             .new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             .source_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                             .destination_access_mask =
                                     VK_ACCESS_TRANSFER_READ_BIT,
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
                    cb.get(), image.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                    VK_FILTER_LINEAR);

            cb.pipeline_barrier(
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    std::array{image.transition(
                            {.old_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             .new_layout =
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             .source_access_mask = VK_ACCESS_TRANSFER_READ_BIT,
                             .destination_access_mask =
                                     VK_ACCESS_SHADER_READ_BIT,
                             .base_mip_level = i - 1})});

            mip_width = next_mip_width;
            mip_height = next_mip_height;
        }

        cb.pipeline_barrier(
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                std::array{image.transition(
                        {.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         .new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         .source_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                         .destination_access_mask = VK_ACCESS_SHADER_READ_BIT,
                         .base_mip_level = mip_levels - 1})});
    }
}


planet::vk::texture
        planet::vk::texture::create_with_mip_levels_from(texture::args args) {
    auto const mhw = std::max(args.width, args.height);
    std::uint32_t const mip_levels = 1 + std::floor(std::log2(mhw));

    vk::texture texture;

    texture.fit = args.scale;

    texture.image = {
            args.allocator,
            args.width,
            args.height,
            mip_levels,
            VK_SAMPLE_COUNT_1_BIT,
            args.format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    auto cb = planet::vk::command_buffer::single_use(args.command_pool);
    transition_layout(
            cb, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            mip_levels);
    copy_from(cb, texture.image, args.buffer);
    generate_mip_maps(cb, texture.image, mip_levels);
    cb.end_and_submit();

    texture.image_view = {texture.image, VK_IMAGE_ASPECT_COLOR_BIT};

    texture.sampler = {
            {.device = args.allocator.device,
             .mip_levels = mip_levels,
             .address_mode = args.address_mode}};

    return texture;
}


planet::vk::texture planet::vk::texture::create_without_mip_levels_from(
        texture::args args) {
    vk::texture texture;

    texture.fit = args.scale;

    texture.image = {
            args.allocator,
            args.width,
            args.height,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            args.format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    auto cb = planet::vk::command_buffer::single_use(args.command_pool);
    transition_layout(
            cb, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
    copy_from(cb, texture.image, args.buffer);
    cb.pipeline_barrier(
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            std::array{texture.image.transition(
                    {.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     .source_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                     .destination_access_mask = VK_ACCESS_SHADER_READ_BIT})});
    cb.end_and_submit();

    texture.image_view = {texture.image, VK_IMAGE_ASPECT_COLOR_BIT};

    texture.sampler = {
            {.device = args.allocator.device,
             .address_mode = args.address_mode}};

    return texture;
}


planet::vk::texture::operator bool() const noexcept {
    return image.get() and image_view.get() and sampler.get();
}

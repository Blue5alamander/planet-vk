#include <planet/vk/commands.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/texture.hpp>

#include <cmath>


/// ## `planet::vk::sampler`


planet::vk::sampler::sampler(vk::device &d, std::uint32_t const mip_levels)
: device{d} {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy =
            device().instance.gpu().properties.limits.maxSamplerAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.minLod = 0.0f;
    info.maxLod = static_cast<float>(mip_levels);
    info.mipLodBias = 0.0f;

    handle.create<vkCreateSampler>(device.get(), info);
}


/// ## `planet::vk::texture`


planet::vk::texture planet::vk::texture::create_with_mip_levels_from(
        device_memory_allocator &allocator,
        command_pool &cp,
        vk::buffer<std::byte> const &buffer,
        std::uint32_t const width,
        std::uint32_t const height,
        ui::scale const fit) {
    auto const mhw = std::max(width, height);
    std::uint32_t const mip_levels = 1 + std::floor(std::log2(mhw));

    vk::texture texture;

    texture.fit = fit;

    texture.image = {
            allocator,
            width,
            height,
            mip_levels,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    texture.image.transition_layout(
            cp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            mip_levels);
    texture.image.copy_from(cp, buffer);
    texture.image.generate_mip_maps(cp, mip_levels);

    texture.image_view = {texture.image, VK_IMAGE_ASPECT_COLOR_BIT};

    texture.sampler = {allocator.device, mip_levels};

    return texture;
}


planet::vk::texture planet::vk::texture::create_without_mip_levels_from(
        device_memory_allocator &allocator,
        command_pool &cp,
        vk::buffer<std::byte> const &buffer,
        std::uint32_t const width,
        std::uint32_t const height,
        ui::scale const fit) {
    vk::texture texture;

    texture.fit = fit;

    texture.image = {
            allocator,
            width,
            height,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    texture.image.transition_layout(
            cp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1);
    texture.image.copy_from(cp, buffer);

    auto cb = planet::vk::command_buffer::single_use(cp);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = texture.image.get();
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
            cb.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &barrier);
    cb.end_and_submit();

    texture.image_view = {texture.image, VK_IMAGE_ASPECT_COLOR_BIT};

    texture.sampler = {allocator.device, 1};

    return texture;
}


planet::vk::texture::operator bool() const noexcept {
    return image.get() and image_view.get() and sampler.get();
}

#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/texture.hpp>


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

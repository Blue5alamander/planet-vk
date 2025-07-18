#pragma once


#include <planet/vk/device.hpp>


namespace planet::vk {


    /// ## Texture UBO
    /**
     * A UBO that allows one out of a number of textures to be used in the
     * fragment shader. Used for things like sprites and text written on quads.
     */
    struct textures {
        vk::device &device;
        vk::descriptor_set_layout layout = [this]() {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = 0;
            binding.descriptorCount = 1;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.pImmutableSamplers = nullptr;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            return vk::descriptor_set_layout{device, binding};
        }();
        std::vector<VkDescriptorImageInfo> descriptors = {};
    };


}

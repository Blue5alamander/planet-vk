#pragma once


namespace planet::vk {


    /// ## Texture UBO
    /**
     * A UBO that allows one out of a number of textures to be used in the
     * fragment shader. Used for things like sprites and text written on quads.
     */
    struct textures {
        std::vector<VkDescriptorImageInfo> descriptors;
    };


}

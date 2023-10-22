#pragma once


#include <planet/vk/image.hpp>

#include <array>
#include <span>


namespace planet::vk::engine {


    struct depth_buffer {
        depth_buffer(swap_chain &);

        vk::image image;
        vk::image_view image_view;


        static VkFormat default_format(physical_device const &);

        static VkFormat find_supported_format(
                physical_device const &,
                std::span<VkFormat> candidates,
                VkImageTiling,
                VkFormatFeatureFlags);

        template<std::size_t N>
        static VkFormat find_supported_format(
                physical_device const &pd,
                std::array<VkFormat, N> candidates,
                VkImageTiling const t,
                VkFormatFeatureFlags const f) {
            return find_supported_format(pd, std::span{candidates}, t, f);
        }

        VkAttachmentDescription
                attachment_description(physical_device const &) const;
    };


    inline bool has_stencil_component(VkFormat const f) {
        return f == VK_FORMAT_D32_SFLOAT_S8_UINT
                or f == VK_FORMAT_D24_UNORM_S8_UINT;
    }


}

#pragma once


#include <planet/vk/image.hpp>

#include <array>
#include <span>


namespace planet::vk::engine {


    struct depth_buffer {
        vk::image image;
        vk::image_view image_view;


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
    };


}

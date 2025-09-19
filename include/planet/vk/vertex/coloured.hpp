#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/vertex/forward.hpp>


namespace planet::vk::vertex {


    struct coloured {
        planet::affine::point3d p;
        colour c;
    };

    template<>
    inline constexpr auto binding_description<coloured>() {
        return std::array{VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(coloured),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    template<>
    inline constexpr auto attribute_description<coloured>() {
        return std::array{
                VkVertexInputAttributeDescription{
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(coloured, p)},
                VkVertexInputAttributeDescription{
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(coloured, c)}};
    }


}

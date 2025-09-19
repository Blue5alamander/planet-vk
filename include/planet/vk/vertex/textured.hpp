#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/vertex/forward.hpp>


namespace planet::vk::vertex {


    /// ## UV texture position
    struct pos2d {
        float x = {}, y = {};
        friend constexpr pos2d operator+(pos2d const l, pos2d const r) {
            return {l.x + r.x, l.y + r.y};
        }
    };


    /// ## Basic textured mesh vertex
    struct textured {
        planet::affine::point3d p;
        colour col = colour::white;
        pos2d uv;
    };

    template<>
    inline constexpr auto binding_description<textured>() {
        return std::array{VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(textured),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    template<>
    inline constexpr auto attribute_description<textured>() {
        return std::array{
                VkVertexInputAttributeDescription{
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(textured, p)},
                VkVertexInputAttributeDescription{
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(textured, col)},
                VkVertexInputAttributeDescription{
                        .location = 2,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(textured, uv)}};
    }


}

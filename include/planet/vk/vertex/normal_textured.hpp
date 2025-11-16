#pragma once


#include <planet/vertex/normal_textured.hpp>
#include <planet/vk/vertex/bindings.hpp>

#include <vulkan/vulkan.h>


namespace planet::vertex {


    template<>
    inline constexpr auto binding_description<normal_textured>() {
        return std::array{VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(normal_textured),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    template<>
    inline constexpr auto attribute_description<normal_textured>() {
        return std::array{
                VkVertexInputAttributeDescription{
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(normal_textured, p)},
                VkVertexInputAttributeDescription{
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(normal_textured, n)},
                VkVertexInputAttributeDescription{
                        .location = 2,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(normal_textured, uv)}};
    }


}

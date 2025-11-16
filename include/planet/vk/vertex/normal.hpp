#pragma once


#include <planet/vertex/normal.hpp>
#include <planet/vk/vertex/bindings.hpp>

#include <vulkan/vulkan.h>


namespace planet::vertex {


    template<>
    inline constexpr auto binding_description<normal>() {
        return std::array{VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(normal),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    template<>
    inline constexpr auto attribute_description<normal>() {
        return std::array{
                VkVertexInputAttributeDescription{
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(normal, p)},
                VkVertexInputAttributeDescription{
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(normal, n)}};
    }


}

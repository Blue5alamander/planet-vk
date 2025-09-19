#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/serialise/forward.hpp>
#include <planet/vk/vertex/forward.hpp>


namespace planet::vk::vertex {


    struct normal {
        constexpr static std::string_view box{"_p:vk:vert:n"};


        affine::point3d p, n;
    };
    void save(serialise::save_buffer &, normal const &);
    void load(serialise::box &, normal &);


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

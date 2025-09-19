#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/serialise/forward.hpp>
#include <planet/vk/vertex/forward.hpp>
#include <planet/vk/vertex/uv.hpp>


namespace planet::vk::vertex {


    struct normal_textured {
        constexpr static std::string_view box{"_p:vk:vert:ntx"};


        affine::point3d p, n;
        uvpos uv;
    };
    void save(serialise::save_buffer &, normal_textured const &);
    void load(serialise::box &, normal_textured &);


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

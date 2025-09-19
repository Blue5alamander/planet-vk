#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/vertex/forward.hpp>
#include <planet/vk/vertex/uv.hpp>


namespace planet::vk::vertex {


    /// ## Basic textured mesh vertex
    struct textured {
        constexpr static std::string_view box{"_p:vk:vert:tx"};


        planet::affine::point3d p;
        colour col = colour::white;
        uvpos uv;
    };
    void save(serialise::save_buffer &, textured const &);
    void load(serialise::box &, textured &);

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

#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/vertex/forward.hpp>
#include <planet/vk/vertex/uv.hpp>


namespace planet::vk::vertex {


    /// ## Basic textured mesh vertex with colour
    struct coloured_textured {
        constexpr static std::string_view box{"_p:vk:vert:ctx"};


        planet::affine::point3d p;
        colour col = colour::white;
        uvpos uv;
    };
    void save(serialise::save_buffer &, coloured_textured const &);
    void load(serialise::box &, coloured_textured &);

    template<>
    inline constexpr auto binding_description<coloured_textured>() {
        return std::array{VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(coloured_textured),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    template<>
    inline constexpr auto attribute_description<coloured_textured>() {
        return std::array{
                VkVertexInputAttributeDescription{
                        .location = 0,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(coloured_textured, p)},
                VkVertexInputAttributeDescription{
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(coloured_textured, col)},
                VkVertexInputAttributeDescription{
                        .location = 2,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(coloured_textured, uv)}};
    }


}

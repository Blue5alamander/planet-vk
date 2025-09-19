#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/array.hpp>
#include <planet/log.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/vertex/textured.hpp>


namespace planet::vk::ubo {


    /// ## Texture UBO
    /**
     * A UBO that allows one out of a number of textures to be used in the
     * fragment shader. Used for things like sprites and text written on quads.
     */
    template<typename Vertex, std::size_t Frames>
    struct textures final {
        using vertex_type = Vertex;


        textures(
                std::string_view const name,
                vk::device &d,
                std::uint32_t max_textures_per_frame)
        : device{d},
          max_per_frame{max_textures_per_frame},
          textures_in_frame{std::string{name} + "__textures_in_frame"} {}


        /// ### Configuration
        vk::device &device;
        std::uint32_t max_per_frame;
        telemetry::max textures_in_frame;


        /// ### Vulkan set up
        vk::descriptor_set_layout layout{
                device,
                {.binding = 0,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr}};
        vk::descriptor_pool pool{
                device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                static_cast<std::uint32_t>(Frames *max_per_frame)};
        std::array<vk::descriptor_sets, Frames> sets = array_of<Frames>([&]() {
            return vk::descriptor_sets{pool, layout, max_per_frame};
        });
    };


}

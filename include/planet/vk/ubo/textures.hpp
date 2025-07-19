#pragma once


#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>


namespace planet::vk {


    /// ## Texture UBO
    /**
     * A UBO that allows one out of a number of textures to be used in the
     * fragment shader. Used for things like sprites and text written on quads.
     */
    template<std::size_t Frames>
    struct textures {
        textures(vk::device &d, std::uint32_t max_textures_per_frame)
        : device{d}, max_per_frame{max_textures_per_frame} {
            for (std::size_t index{}; index < Frames; ++index) {
                sets[index] = {pool, layout, max_per_frame};
            }
        }


        /// ### Configuration
        vk::device &device;
        std::uint32_t max_per_frame;


        /// ### Vulkan set up
        vk::descriptor_set_layout layout = [this]() {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = 0;
            binding.descriptorCount = 1;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.pImmutableSamplers = nullptr;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            return vk::descriptor_set_layout{device, binding};
        }();
        vk::descriptor_pool pool{
                device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                static_cast<std::uint32_t>(Frames *max_per_frame)};
        std::array<vk::descriptor_sets, Frames> sets = {};


        /// ### Vertex data format
        struct pos {
            float x = {}, y = {};
            friend constexpr pos operator+(pos const l, pos const r) {
                return {l.x + r.x, l.y + r.y};
            }
        };
        struct vertex {
            planet::affine::point3d p;
            colour col = colour::white;
            pos uv;
        };

        static constexpr auto binding_description{[]() {
            VkVertexInputBindingDescription description{};

            description.binding = 0;
            description.stride =
            sizeof(vertex);
            description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return std::array{description};
        }()};
        static constexpr auto attribute_description{[]() {
            std::array<VkVertexInputAttributeDescription, 3> attrs{};

            attrs[0].binding = 0;
            attrs[0].location = 0;
            attrs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attrs[0].offset =
            offsetof(vertex, p);

            attrs[1].binding = 0;
            attrs[1].location = 1;
            attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attrs[1].offset =
            offsetof(vertex, col);

            attrs[2].binding = 0;
            attrs[2].location = 2;
            attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
            attrs[2].offset =
            offsetof(vertex, uv);

            return attrs;
        }()};


        /// ### Across frames data
        std::array<buffer<vertex>, Frames> vertex_buffers;
        std::array<buffer<std::uint32_t>, Frames> index_buffers;


        /// ### Per-frame data
        std::vector<VkDescriptorImageInfo> descriptors = {};
    };


}

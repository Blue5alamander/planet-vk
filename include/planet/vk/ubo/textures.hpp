#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/log.hpp>
#include <planet/telemetry/minmax.hpp>
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
        textures(
                std::string_view const name,
                vk::device &d,
                std::uint32_t max_textures_per_frame)
        : device{d},
          max_per_frame{max_textures_per_frame},
          textures_in_frame{std::string{name} + "__textures_in_frame"} {
            for (std::size_t index{}; index < Frames; ++index) {
                sets[index] = {pool, layout, max_per_frame};
            }
        }


        /// ### Configuration
        vk::device &device;
        std::uint32_t max_per_frame;
        telemetry::max textures_in_frame;


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
            description.stride = sizeof(vertex);
            description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return std::array{description};
        }()};
        static constexpr auto attribute_description{[]() {
            std::array<VkVertexInputAttributeDescription, 3> attrs{};

            attrs[0].binding = 0;
            attrs[0].location = 0;
            attrs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attrs[0].offset = offsetof(vertex, p);

            attrs[1].binding = 0;
            attrs[1].location = 1;
            attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attrs[1].offset = offsetof(vertex, col);

            attrs[2].binding = 0;
            attrs[2].location = 2;
            attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
            attrs[2].offset = offsetof(vertex, uv);

            return attrs;
        }()};


        /// ### Across frames data
        std::array<buffer<vertex>, Frames> vertex_buffers;
        std::array<buffer<std::uint32_t>, Frames> index_buffers;


        /// ### Per-frame data
        std::vector<VkDescriptorImageInfo> descriptors = {};
        std::vector<vertex> vertices;
        std::vector<std::uint32_t> indices;


        /// ### Upload buffers to GPU
        [[nodiscard]] bool
                bind(device_memory_allocator &allocator,
                     std::size_t frame_index,
                     command_buffer &cb)
        /**
         * Returns `true` if there are vertices and indices and they have been
         * uploaded to the GPU. If `false` then there is nothing more to do in
         * any render calls for pipelines using this UBO.
         */
        {
            if (empty()) { return false; }

            auto &vertex_buffer = vertex_buffers[frame_index];
            vertex_buffer = {
                    allocator, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
            auto &index_buffer = index_buffers[frame_index];
            index_buffer = {
                    allocator, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

            std::array buffers{vertex_buffer.get()};
            std::array offset{VkDeviceSize{}};

            vkCmdBindVertexBuffers(
                    cb.get(), 0, buffers.size(), buffers.data(), offset.data());
            vkCmdBindIndexBuffer(
                    cb.get(), index_buffer.get(), 0, VK_INDEX_TYPE_UINT32);

            textures_in_frame.value(descriptors.size());
            if (descriptors.size() > max_per_frame) {
                planet::log::critical(
                        "We will run out of texture slots for this frame");
            }
            return true;
        }


        void clear() {
            vertices.clear();
            indices.clear();
            descriptors.clear();
        }
        [[nodiscard]] bool empty() const noexcept { return vertices.empty(); }
    };


}

#pragma once


#include <planet/affine/point3d.hpp>
#include <planet/log.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/engine/forward.hpp>
#include <planet/vk/vertex/textured.hpp>


namespace planet::vk::ubo {


    /// ## Texture UBO
    /**
     * A UBO that allows one out of a number of textures to be used in the
     * fragment shader. Used for things like sprites and text written on quads.
     */
    template<
            typename Vertex = vertex::textured,
            std::size_t Frames = engine::max_frames_in_flight>
    struct textures final {
        using vertex_type = Vertex;


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
        std::array<vk::descriptor_sets, Frames> sets = {};


        /// ### Across frames data
        std::array<buffer<vertex_type>, Frames> vertex_buffers;
        std::array<buffer<std::uint32_t>, Frames> index_buffers;


        /// ### Per-frame data
        std::vector<VkDescriptorImageInfo> descriptors = {};
        std::vector<vertex_type> vertices;
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

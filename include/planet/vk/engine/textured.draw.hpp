#pragma once


#include <planet/telemetry/minmax.hpp>
#include <planet/vk/ubo/textures.hpp>
#include <planet/vk/vertex/coloured_textured.hpp>


namespace planet::vk::engine {


    template<std::size_t Frames = engine::max_frames_in_flight>
    struct draw_basic_textures final {
        using vertex_type = vertex::coloured_textured;


        draw_basic_textures(
                std::string_view const name,
                vk::device &d,
                std::uint32_t max_textures_per_frame)
        : ubo{name, d, max_textures_per_frame} {}


        /// ### UBO
        ubo::textures<vertex_type, Frames> ubo;


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

            ubo.textures_in_frame.value(descriptors.size());
            if (descriptors.size() > ubo.max_per_frame) {
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

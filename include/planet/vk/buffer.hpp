#pragma once


#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>


namespace planet::vk {


    /// Vertex buffer
    template<typename Vertex>
    class buffer {
        using buffer_handle_type = device_handle<VkBuffer, vkDestroyBuffer>;
        buffer_handle_type buffer_handle;

        using memory_handle_type = device_handle<VkDeviceMemory, vkFreeMemory>;
        memory_handle_type memory_handle;

        std::size_t count;

        struct mapped_memory {
            vk::device const &device;
            memory_handle_type const &memory;
            void *pointer = nullptr;
            mapped_memory(
                    vk::device const &d,
                    memory_handle_type const &m,
                    VkDeviceSize const offset,
                    VkDeviceSize const size,
                    VkMemoryMapFlags flags = {})
            : device{d}, memory{m} {
                worked(vkMapMemory(
                        device.get(), memory.get(), offset, size, flags,
                        &pointer));
            }
            ~mapped_memory() {
                if (pointer) { vkUnmapMemory(device.get(), memory.get()); }
            }
        };

      public:
        buffer(vk::device const &d,
               std::span<Vertex const> const vertices,
               VkMemoryPropertyFlags const properties =
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        : count{vertices.size()}, device{d} {
            VkBufferCreateInfo buffer{};
            buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer.size = sizeof(Vertex) * vertices.size();
            buffer.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            buffer_handle.create<vkCreateBuffer>(device.get(), buffer);

            VkMemoryAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc.allocationSize = memory_requirements().size;
            alloc.memoryTypeIndex = find_memory_type(properties);
            memory_handle.create<vkAllocateMemory>(device.get(), alloc);

            worked(vkBindBufferMemory(
                    device.get(), buffer_handle.get(), memory_handle.get(), {}));

            mapped_memory data{device, memory_handle, {}, buffer.size};
            std::span<Vertex> gpumemory{
                    reinterpret_cast<Vertex *>(data.pointer), vertices.size()};
            std::copy(vertices.begin(), vertices.end(), gpumemory.begin());
        }

        vk::device const &device;
        VkBuffer get() const noexcept { return buffer_handle.get(); }

        /// Number of items in the buffer
        std::size_t size() const noexcept { return count; }

        /// Return the memory requirements for the buffer
        VkMemoryRequirements memory_requirements() const noexcept {
            VkMemoryRequirements mr{};
            vkGetBufferMemoryRequirements(
                    device.get(), buffer_handle.get(), &mr);
            return mr;
        }
        /// Searches for a memory index that matches the requested properties
        std::uint32_t find_memory_type(VkMemoryPropertyFlags properties) const {
            auto const mr = memory_requirements();
            auto const &gpump = device.instance.gpu().memory_properties;
            for (std::uint32_t index{}; index < gpump.memoryTypeCount;
                 ++index) {
                bool const type_is_correct =
                        (mr.memoryTypeBits bitand (1 << index));
                bool const properties_match =
                        (gpump.memoryTypes[index].propertyFlags
                         bitand properties);
                if (type_is_correct and properties_match) { return index; }
            }
            throw felspar::stdexcept::runtime_error{
                    "No matching GPU memory was found for allocation"};
        }
    };


}

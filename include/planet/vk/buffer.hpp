#pragma once


#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory.hpp>


namespace planet::vk {


    /// A buffer of multiple items
    template<typename T>
    class buffer {
        vk::device const *pdevice = nullptr;

        using buffer_handle_type = device_handle<VkBuffer, vkDestroyBuffer>;
        buffer_handle_type buffer_handle;

        device_memory memory;

        /// The number of T items in the buffer
        std::size_t count;

      public:
        using value_type = T;

        buffer() {}
        buffer(vk::device const &device,
               std::size_t const c,
               VkBufferUsageFlags const usage,
               VkMemoryPropertyFlags const properties)
        : pdevice(&device), count{c} {
            VkBufferCreateInfo buffer{};
            buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer.size = byte_count();
            buffer.usage = usage;
            buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            buffer_handle.create<vkCreateBuffer>(device.get(), buffer);

            memory = device_memory{
                    device, memory_requirements().size,
                    find_memory_type(properties)};

            worked(vkBindBufferMemory(
                    device.get(), buffer_handle.get(), memory.get(), {}));
        }
        buffer(vk::device const &device,
               std::span<T const> const vertices,
               VkBufferUsageFlags const usage,
               VkMemoryPropertyFlags const properties)
        : buffer{device, vertices.size(), usage, properties} {
            auto data = memory.map_memory({}, byte_count());
            std::span<value_type> gpumemory{
                    reinterpret_cast<value_type *>(data.pointer),
                    vertices.size()};
            std::copy(vertices.begin(), vertices.end(), gpumemory.begin());
        }

        VkBuffer get() const noexcept { return buffer_handle.get(); }

        /// Number of items in the buffer
        std::size_t size() const noexcept { return count; }
        std::size_t byte_count() const noexcept { return count * sizeof(T); }

        /// Return the memory requirements for the buffer
        VkMemoryRequirements memory_requirements() const noexcept {
            VkMemoryRequirements mr{};
            vkGetBufferMemoryRequirements(
                    buffer_handle.owner(), buffer_handle.get(), &mr);
            return mr;
        }
        /// Searches for a memory index that matches the requested properties
        std::uint32_t find_memory_type(VkMemoryPropertyFlags properties) const {
            auto const mr = memory_requirements();
            auto const &gpump = pdevice->instance.gpu().memory_properties;
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

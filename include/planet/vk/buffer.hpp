#pragma once


#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory.hpp>


namespace planet::vk {


    /// A buffer of multiple items
    template<typename T>
    class buffer {
        device_memory_allocator *allocator = nullptr;

        using buffer_handle_type = device_handle<VkBuffer, vkDestroyBuffer>;
        buffer_handle_type buffer_handle;

        device_memory memory;

        /// The number of T items in the buffer
        std::size_t count;

      public:
        using value_type = T;

        /// ### Constructors

        buffer() {}
        /// #### Allocate space for a number of items
        buffer(device_memory_allocator &a,
               std::size_t const c,
               VkBufferUsageFlags const usage,
               VkMemoryPropertyFlags const properties)
        : allocator(&a), count{c} {
            VkBufferCreateInfo buffer{};
            buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer.size = byte_count();
            buffer.usage = usage;
            buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            buffer_handle.create<vkCreateBuffer>(
                    allocator->device.get(), buffer);

            auto mr = memory_requirements();
            memory = allocator->allocate(
                    byte_count(), find_memory_type(properties), mr.alignment);
            memory.bind_buffer_memory(buffer_handle.get());
        }
        /// #### Allocate memory for a number of items
        /// And copy them into GPU memory
        buffer(device_memory_allocator &allocator,
               std::span<T const> const items,
               VkBufferUsageFlags const usage,
               VkMemoryPropertyFlags const properties)
        : buffer{allocator, items.size(), usage, properties} {
            auto data = memory.map_memory({}, byte_count());
            std::span<value_type> gpumemory{
                    reinterpret_cast<value_type *>(data.get()), items.size()};
            std::copy(items.begin(), items.end(), gpumemory.begin());
        }


        /// ### Queries
        VkBuffer get() const noexcept { return buffer_handle.get(); }

        /// #### Number of items in the buffer
        std::size_t size() const noexcept { return count; }
        std::size_t byte_count() const noexcept { return count * sizeof(T); }

        /// #### Return the memory requirements for the buffer
        VkMemoryRequirements memory_requirements() const noexcept {
            VkMemoryRequirements mr{};
            vkGetBufferMemoryRequirements(
                    buffer_handle.owner(), buffer_handle.get(), &mr);
            return mr;
        }
        /// #### Searches for a memory index that matches the requested properties
        std::uint32_t
                find_memory_type(VkMemoryPropertyFlags const properties) const {
            return allocator->device().instance.find_memory_type(
                    memory_requirements(), properties);
        }


        /// ### Map GPU memory to host
        auto map() { return memory.map_memory({}, byte_count()); }
    };


}

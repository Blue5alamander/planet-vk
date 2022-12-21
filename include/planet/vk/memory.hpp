#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    /// A raw allocation of GPU memory
    /**
     */
    /// TODO Have the memory managed by a separate allocator that splits a
    /// large allocation
    class device_memory {
        using handle_type = device_handle<VkDeviceMemory, vkFreeMemory>;
        handle_type handle;

      public:
        device_memory() {}
        device_memory(
                vk::device const &,
                std::size_t bytes,
                std::uint32_t memory_type_index);

        device_memory(device_memory &&) = default;
        device_memory &operator=(device_memory &&) = default;

        VkDeviceMemory get() const noexcept { return handle.get(); }

        /// Map all/some of the memory to system RAM
        auto map_memory(
                VkDeviceSize const offset,
                VkDeviceSize const size,
                VkMemoryMapFlags flags = {}) {
            struct mapped_memory {
                VkDevice const device_handle;
                VkDeviceMemory const memory_handle;
                void *const pointer;
                mapped_memory(
                        VkDevice const d,
                        VkDeviceMemory const m,
                        VkDeviceSize const offset,
                        VkDeviceSize const size,
                        VkMemoryMapFlags flags = {})
                : device_handle{d}, memory_handle{m}, pointer{[&]() {
                      void *p = nullptr;
                      worked(vkMapMemory(
                              device_handle, memory_handle, offset, size, flags,
                              &p));
                      return p;
                  }()} {}
                ~mapped_memory() {
                    if (pointer) {
                        vkUnmapMemory(device_handle, memory_handle);
                    }
                }
            };
            return mapped_memory{
                    handle.owner(), handle.get(), offset, size, flags};
        }
    };


};

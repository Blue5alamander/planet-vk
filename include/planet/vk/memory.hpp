#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    /// ## Allocated GPU memory
    /// TODO Have the memory managed by a separate allocator that splits a
    /// large allocation
    class device_memory final {
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
        class mapping;
        friend class device_memory::mapping;
        mapping map_memory(
                VkDeviceSize const offset,
                VkDeviceSize const size,
                VkMemoryMapFlags flags = {});
    };


    /// ## CPU memory mapped to GPU memory
    class device_memory::mapping final {
        VkDevice device_handle;
        VkDeviceMemory memory_handle;
        void *pointer = nullptr;

        void unsafe_reset();

      public:
        mapping();
        mapping(mapping const &) = delete;
        mapping(mapping &&);
        mapping(VkDevice const d,
                VkDeviceMemory const m,
                VkDeviceSize const offset,
                VkDeviceSize const size,
                VkMemoryMapFlags flags = {});
        ~mapping() { unsafe_reset(); }

        mapping &operator=(mapping const &) = delete;
        mapping &operator=(mapping &&);

        void *get() const noexcept { return pointer; }
    };


};

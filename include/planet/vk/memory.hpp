#pragma once


#include <planet/vk/helpers.hpp>

#include <atomic>
#include <mutex>


namespace planet::vk {


    class device;
    class device_memory;
    class device_memory_allocator;


    /// ## Memory allocation configuration
    struct device_memory_allocator_configuration {
        /// ### Minimum GPU device memory alignment
        std::size_t minimum_alignment = 1 << 10;
        /// ### GPU memory allocation block size
        std::size_t allocation_block_size = 64 << 20;
    };
    /// ### Default "safe" configuration for an allocator
    static constexpr device_memory_allocator_configuration
            thread_safe_device_memory_allocator = {};


    /// ## Allocated GPU memory
    /**
     * These larger allocations are intended to be sub-divided into individual
     * sub-allocations that are then tracked by the `device_memory` instances.
     */
    class device_memory_allocation final {
        friend class device_memory;
        friend class device_memory_allocator;
        using handle_type = device_handle<VkDeviceMemory, vkFreeMemory>;
        handle_type handle;
        std::atomic<std::size_t> ownership_count = 1u;
        device_memory_allocator *allocator = nullptr;
        std::uint32_t memory_type_index = {};

        device_memory_allocation(
                device_memory_allocator *a,
                handle_type h,
                std::uint32_t const mti)
        : handle{std::move(h)}, allocator{a}, memory_type_index{mti} {}

      public:
        ~device_memory_allocation();

        device_memory_allocation(device_memory_allocation &&) = delete;
        device_memory_allocation(device_memory_allocation const &) = delete;
        device_memory_allocation &
                operator=(device_memory_allocation &&) = delete;
        device_memory_allocation &
                operator=(device_memory_allocation const &) = delete;

        VkDeviceMemory get() const noexcept { return handle.get(); }

        /// ### Perform raw memory allocation
        static handle_type allocate(
                vk::device const &,
                std::size_t bytes,
                std::uint32_t memory_type_index);

        /// ### Handle increment and decrement counts
        static device_memory_allocation *
                increment(device_memory_allocation *) noexcept;
        static void decrement(device_memory_allocation *&) noexcept;
    };


    /// ## A sub-chunk of allocated GPU memory
    /// TODO Have the memory managed by a separate allocator that splits a
    /// large allocation
    class device_memory final {
        friend class device_memory_allocator;

        device_memory_allocation *allocation = nullptr;
        // std::size_t offset = {};
        // std::size_t size = {};

        /// The allocated memory provided here
        device_memory(device_memory_allocation *a) noexcept : allocation{a} {}

      public:
        device_memory() {}
        ~device_memory() { reset(); }

        device_memory(device_memory &&);
        device_memory(device_memory const &);
        device_memory &operator=(device_memory &&);
        device_memory &operator=(device_memory const &);

        /// ### Free the held memory
        void reset() { device_memory_allocation::decrement(allocation); }

        /// ### Return the Vulkan handle to the memory
        VkDeviceMemory get() const noexcept {
            return allocation ? allocation->get() : VK_NULL_HANDLE;
        }

        /// ### Map all/some of the memory to system RAM
        class mapping;
        friend class device_memory::mapping;
        mapping map_memory(
                VkDeviceSize const offset,
                VkDeviceSize const size,
                VkMemoryMapFlags flags = {});
    };


    /// ## Allocators for GPU memory
    /**
     * Instances of this type are used to allocate memory for the game.
     * Different instances may have different expectations of the frequency and
     * use of the handed out memory.
     */
    class device_memory_allocator final {
        /// ### Memory pool
        struct pool {
            std::mutex mtx;
            std::vector<device_memory_allocation::handle_type> free_memory;
        };
        /// ### Free memory by memory type index
        std::vector<pool> pools;

      public:
        /// ### Bind allocator to a device
        device_memory_allocator(
                vk::device const &,
                device_memory_allocator_configuration const & =
                        thread_safe_device_memory_allocator);

        vk::device const &device;
        device_memory_allocator_configuration config;

        /// ### Allocation and release

        /// #### Allocate memory from this allocator's pool
        device_memory allocate(
                std::size_t const bytes, std::uint32_t const memory_type_index);

        /// #### Release memory
        void deallocate(
                device_memory_allocation::handle_type,
                std::uint32_t memory_type_index);


        /// ### Clear all memory held by the allocator
        /**
         * This will be done automatically by the destructor, but this allows
         * the allocator to be cleared with a check that any outstanding
         * allocations have been properly returned.
         */
        void clear_without_check();
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

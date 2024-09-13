#pragma once


#include <planet/telemetry/counter.hpp>
#include <planet/telemetry/id.hpp>
#include <planet/vk/forward.hpp>
#include <planet/vk/owned_handle.hpp>
#include <planet/vk/view.hpp>

#include <atomic>
#include <mutex>


namespace planet::vk {


    /// ## Memory allocation configuration
    struct device_memory_allocator_configuration {
        /// ### Minimum GPU device memory alignment
        std::size_t minimum_alignment = 1 << 10;
        /// ### GPU memory allocation block size
        std::size_t allocation_block_size = 64 << 20;
        /// ### Whether this allocator should split memory allocations
        bool split = true;
        /// ### Memory mapping flags for all memory allocated here
        VkMemoryMapFlags memory_map_flags = {};
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

        static void deallocate(
                VkDevice, VkDeviceMemory, VkAllocationCallbacks const *);
        using handle_type = device_handle<VkDeviceMemory, &deallocate>;
        handle_type handle;

        std::atomic<std::size_t> ownership_count = 1u;
        device_memory_allocator *allocator = nullptr;
        std::uint32_t memory_type_index = {};
        std::size_t allocation_size = {};

        std::mutex mapping_mtx;
        std::size_t mapping_count = 0u;
        std::byte *mapped_base = nullptr;

        device_memory_allocation(
                device_memory_allocator *,
                handle_type,
                std::uint32_t,
                std::size_t);


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
                vk::device &,
                std::size_t bytes,
                std::uint32_t memory_type_index);


        /// ### Handle increment and decrement counts

        /// #### Increment and decrement the ownership counts
        static device_memory_allocation *
                increment(device_memory_allocation *) noexcept;
        static void decrement(device_memory_allocation *&) noexcept;

        /// #### Increment and decrement the mapping count
        std::byte *map_memory();
        void unmap_memory();
    };


    /// ## A sub-chunk of allocated GPU memory
    class device_memory final {
        friend class device_memory_allocator;

        device_memory_allocation *allocation = nullptr;
        std::size_t offset = {};
        std::size_t byte_count = {};

        /// The allocated memory provided here
        device_memory(
                device_memory_allocation *a,
                std::size_t const o,
                std::size_t const s) noexcept
        : allocation{a}, offset{o}, byte_count{s} {}

      public:
        device_memory() {}
        ~device_memory() { reset(); }

        device_memory(device_memory &&);
        device_memory(device_memory const &);
        device_memory &operator=(device_memory &&);
        device_memory &operator=(device_memory const &);

        /// ### Split the memory
        /**
         * Returns a new `device_memory` comprising the first part of this
         * memory and updates the remaining memory to point to the second half
         * of GPU memory.
         */
        device_memory split(std::size_t bytes, std::size_t alignment);


        /// ### Free the held memory
        void reset() {
            device_memory_allocation::decrement(allocation);
            offset = {};
            byte_count = {};
        }


        /// ### Query information about this memory

        /// #### Return the Vulkan handle to the memory
        VkDeviceMemory get() const noexcept {
            return allocation ? allocation->get() : VK_NULL_HANDLE;
        }

        /// #### Return the size of the allocation
        std::size_t size() const noexcept { return byte_count; }


        /// ### Bindings

        /// #### Bind a buffer to this memory
        /**
         * Wrapper around `vkBindBufferMemory` which ensures that the correct
         * memory offset for shared allocations is used.
         */
        void bind_buffer_memory(VkBuffer);

        /// #### Bind an image to this memory
        void bind_image_memory(VkImage);


        /// ### Map all/some of the memory to system RAM
        /**
         * The offset and size parameters are relative to the start of this part
         * of the memory allocation.
         *
         * This is internally reference counted and the entire memory area is
         * mapped on first mapping. This allows the same Vulkan allocation to be
         * mapped multiple times at the same time.
         */
        class mapping;
        friend class device_memory::mapping;
        mapping map_memory(VkDeviceSize offset, VkDeviceSize size);
    };


    /// ## Allocators for GPU memory
    /**
     * Instances of this type are used to allocate memory for the game.
     * Different instances may have different expectations of the frequency and
     * use of the handed out memory.
     */
    class device_memory_allocator final : private telemetry::id {
        friend class device_memory_allocation;


        /// ### Memory pool
        struct pool {
            /**
             * This mutex must be recursive because we can free the rest of the
             * split memory if it is not large enough, and that in turn can lead
             * to a memory chunk being deallocated and added back into the free
             * memory pool -- we really want to be able to re-use that memory,
             * and we really don't want to lock up the freeing code waiting on
             * the same mutex as the allocating code.
             */
            std::recursive_mutex mtx;
            std::vector<device_memory_allocation::handle_type> free_memory;
            device_memory splitting;
        };


        /// ### Free memory by memory type index
        std::vector<pool> pools;


      public:
        /// ### Bind allocator to a device
        device_memory_allocator(
                vk::device &,
                device_memory_allocator_configuration const & =
                        thread_safe_device_memory_allocator);
        device_memory_allocator(
                std::string_view name,
                vk::device &,
                device_memory_allocator_configuration const & =
                        thread_safe_device_memory_allocator,
                id::suffix = id::suffix::no);
        ~device_memory_allocator();


        device_view device;
        device_memory_allocator_configuration config;


        /// ### Allocation and release

        /// #### Allocate memory from this allocator's pool
        device_memory allocate(
                std::size_t bytes,
                std::uint32_t memory_type_index,
                std::size_t alignment);
        device_memory allocate(VkMemoryRequirements, VkMemoryPropertyFlags);

        /// #### Release memory
        void deallocate(
                device_memory_allocation::handle_type,
                std::uint32_t memory_type_index,
                std::size_t bytes);


        /// ### Clear all memory held by the allocator
        /**
         * This allows the allocator to be cleared with a check that any
         * outstanding allocations have been properly returned.
         *
         * This will be done automatically by the destructor.
         */
        void clear_without_check();


      private:
        telemetry::counter c_block_allocation_from_driver,
                c_block_allocation_from_free_list,
                c_block_deallocated_added_to_free_list,
                c_block_deallocation_returned_to_driver,
                c_memory_allocation_count, c_memory_deallocation_count;
    };


    /// ## CPU memory mapped to GPU memory
    class device_memory::mapping final {
        device_memory_allocation *allocation = nullptr;
        void *pointer = nullptr;

        void unsafe_reset();

      public:
        mapping() {}
        mapping(mapping const &) = delete;
        mapping(mapping &&);
        mapping(device_memory_allocation *allocation,
                VkDeviceSize const offset,
                VkDeviceSize const size);
        ~mapping() { unsafe_reset(); }

        mapping &operator=(mapping const &) = delete;
        mapping &operator=(mapping &&);

        std::byte *get() const noexcept {
            return reinterpret_cast<std::byte *>(pointer);
        }
    };


};

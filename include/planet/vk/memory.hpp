#pragma once


#include <planet/telemetry/counter.hpp>
#include <planet/telemetry/id.hpp>
#include <planet/telemetry/map.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/forward.hpp>
#include <planet/vk/owned_handle.hpp>
#include <planet/vk/view.hpp>

#include <atomic>
#include <mutex>


namespace planet::vk {


    /// ## GPU memory allocation strategy
    /**
     * Rather than try to be clever about memory allocation strategies, this
     * model is quite stupid, and relies on the `device_memory_allocator` being
     * appropriately used.
     *
     * Each one is a simple slab allocator. That is, it gets a chunk of memory
     * and then splits it up as requests for memory sizes come in. It records
     * only how many memory requests have been made for the chunk. When all
     * memory has been returned the entire chunk is recycled and used for the
     * next memory requests.
     *
     * This strategy is very fast, but it has a problem. Keeping a single memory
     * block alive will keep the entire chunk from being recycled. Because of
     * this it is **very important** that the memory lifetimes for everything
     * that come from the same allocator are about the same, so that allocations
     * made at the same time are also freed at about the same time.
     *
     * By doing this the allocators can recycle their held memory efficiently,
     * which in turn means that less GPU memory will be needed overall.
     *
     * Memory allocated by the allocators in this file is backed by the
     * `device_memory_block_pool` that exists on the Vulkan device. This is a
     * thread safe pool of memory allocations that means that short lived
     * allocators don't thrash the Vulkan memory APIs.
     */


    /// ## Memory allocation configuration
    struct device_memory_allocator_configuration {
        /// ### GPU memory allocation block size
        std::size_t allocation_block_size = 64 << 20;
        /**
         * Memory requests larger than this can be allocated, but they will
         * always go to the `device_memory_block_pool` pool and will never be
         * recycled by the local allocator.
         */

        /// ### Whether this allocator should split memory allocations
        bool split = true;
        /**
         * This turns off the slab memory allocations in this allocator so it
         * always returns blocks of the `allocation_block_size` for all memory
         * requests.
         */

        /// ### Memory mapping flags for all memory allocated here
        VkMemoryMapFlags memory_map_flags = {};
    };


    /// ## Allocated GPU memory
    /**
     * These larger allocations are intended to be sub-divided into individual
     * sub-allocations that are then tracked by the `device_memory` instances.
     */
    class device_memory_allocation final {
        friend class device_memory;
        friend class device_memory_allocator;
        friend class device_memory_block_pool;

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
        device_memory(device_memory const &) = delete;
        device_memory &operator=(device_memory &&);
        device_memory &operator=(device_memory const &) = delete;


        /// ### Split the memory
        /**
         * Returns a new `device_memory` comprising the first part of this
         * memory and updates the remaining memory to point to the second half
         * of GPU memory.
         */
        device_memory split(std::size_t bytes, std::size_t alignment);


        /// ### Whether a `split` of `bytes` at `alignment` would fit
        /**
         * `split` realigns the current offset up to `alignment`, so it needs
         * `bytes` plus that alignment padding -- which can exceed `size()`. The
         * allocator uses this to decide whether to carve the request from this
         * block or pull a fresh one, instead of letting `split` throw on a
         * request that a fresh block could satisfy.
         */
        bool can_split(std::size_t bytes, std::size_t alignment) const noexcept;


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
         *
         * As a consequence of needing to map the entire underlying block, the
         * `size` passed in is effectively ignored. It should correspond to the
         * memory area wanted anyway.
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
                std::string_view name,
                vk::device &,
                device_memory_allocator_configuration const & = {},
                id::suffix = id::suffix::suppress);
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
         * This will be done automatically by the destructor.
         */
        void clear_without_check();


      private:
        /// ### Telemetry

        /// #### Whole blocks drawn fresh from the shared pool (free-list miss)
        telemetry::counter c_block_allocation_from_device_pool{
                name() + "__block_allocation_from_device_pool"};
        /**
         * Bumped when `allocate` can satisfy a request from neither the
         * splitting block nor the local free list and pulls a fresh whole block
         * from `device_memory_block_pool`.
         */

        /// #### Whole blocks reused from this allocator's local free list (hit)
        telemetry::counter c_block_allocation_from_free_list{
                name() + "__block_allocation_from_free_list"};

        /// #### Whole blocks parked in the local free list on deallocation
        telemetry::counter c_block_deallocated_added_to_free_list{
                name() + "__block_deallocated_added_to_free_list"};
        /**
         * A freed block of size `<= allocation_block_size` is kept in the local
         * free list -- counted live by `c_free_blocks` -- rather than returned
         * to the shared pool.
         */

        /// #### Oversized blocks returned straight to the shared pool
        telemetry::counter c_block_deallocation_returned_to_device_pool{
                name() + "__block_deallocation_returned_to_device_pool"};
        /**
         * A freed block larger than `allocation_block_size` bypasses the local
         * free list and goes back to `device_memory_block_pool` immediately.
         */

        /// #### Ownership references taken on `device_memory_allocation` blocks
        telemetry::counter c_memory_allocation_count{
                name() + "__memory_allocation_count"};
        /**
         * Bumped by the `device_memory_allocation` constructor and by
         * `increment` (which `split` uses), so it counts reference acquisitions
         * on the underlying block, not allocations of memory.
         *
         * TODO Misleading name. `c_memory_allocation_count` minus
         * `c_memory_deallocation_count` is the number of live `device_memory`
         * handles, not a count of `allocate` calls. Rename to something like
         * `c_block_refs_taken`/`c_block_refs_released`.
         */

        /// #### Ownership references released on `device_memory_allocation` blocks
        telemetry::counter c_memory_deallocation_count{
                name() + "__memory_deallocation_count"};
        /**
         * TODO Counterpart to `c_memory_allocation_count`: counts reference
         * releases via `decrement`, not deallocations. Rename alongside it.
         */

        /// #### Per-allocator histogram of requested allocation sizes
        telemetry::map<std::size_t, std::size_t> c_allocation_sizes{
                name() + "__allocation_sizes"};

        /// #### Per-allocator histogram of whole-block sizes pulled from the pool
        telemetry::map<std::size_t, std::size_t> c_device_pool_block_sizes{
                name() + "__device_pool_block_sizes"};

        /// ### Bytes held from the pool, its peak, and the idle-block count
        telemetry::counter c_bytes_held, c_free_blocks;
        telemetry::max c_bytes_held_peak{name() + "__bytes_held_peak"};
        /**
         * Whole blocks are only returned to the shared pool when the allocator
         * is destroyed (or, for oversized blocks, freed early), so
         * `c_bytes_held` trends upwards rather than tracking live use.
         * `c_free_blocks` is the live count of whole blocks sitting idle in
         * this allocator's local free list -- the part of `c_bytes_held` not
         * currently carved into sub-allocations.
         */
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

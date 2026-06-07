#pragma once


#include <planet/telemetry/counter.hpp>
#include <planet/telemetry/id.hpp>
#include <planet/telemetry/map.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/memory.hpp>

#include <cstdint>
#include <map>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>


namespace planet::vk {


    /// ## Shared pool of whole driver memory blocks
    /**
     * A device-wide, thread-safe free list of whole driver allocations, keyed
     * by `(memory_type_index, block_size)`. Allocators draw a recycled block of
     * the right type and size from here instead of pinning their own, and freed
     * blocks survive allocator teardown -- so a short-lived allocator no longer
     * hands its blocks back to the driver only for the next allocator to call
     * `vkAllocateMemory` again.
     *
     * This pool owns the driver-byte accounting: a block counts as in use from
     * the moment it is allocated from the driver until `clear` frees it,
     * whether it is currently checked out to an allocator or sitting in a free
     * list. The telemetry counters are named from the pool's `name`.
     */
    class device_memory_block_pool final : private telemetry::id {
        /// ### A free list of identically keyed whole blocks
        /**
         * Each free list owns its own `std::mutex`, so the lists live in
         * node-stable storage (`std::map`) that never relocates a node once
         * inserted.
         */
        struct free_list final {
            std::mutex mtx;
            std::vector<device_memory_allocation::handle_type> blocks;
        };

        /// ### Free lists keyed by `(memory_type_index, block_size)`
        /**
         * `map_mtx` guards the map structure; each `free_list::mtx` guards its
         * own block vector. The two are never held nested in `acquire`/`release`
         * (the map lock is dropped before a list lock is taken), so the only
         * nested order is `clear`'s `map_mtx` then `free_list::mtx`.
         */
        std::mutex map_mtx;
        std::map<std::pair<std::uint32_t, std::size_t>, free_list> free_lists;

        free_list &list_for(std::uint32_t, std::size_t);


      public:
        /// ### Construct a named pool
        /**
         * The `name` is combined with the pool's type to name the telemetry
         * counters, exactly as `device_memory_allocator` names its own.
         */
        explicit device_memory_block_pool(
                std::string_view name, id::suffix = id::suffix::suppress);


        /// ### Acquire a whole block of the requested type and size
        /**
         * Pops a pooled block of the matching `(memory_type_index, block_size)`
         * or, when none is free, allocates a fresh one from the driver.
         */
        device_memory_allocation::handle_type
                acquire(vk::device &,
                        std::uint32_t memory_type_index,
                        std::size_t block_size);

        /// ### Return a whole block to its free list for later reuse
        void
                release(device_memory_allocation::handle_type,
                        std::uint32_t memory_type_index,
                        std::size_t block_size);

        /// ### Free every pooled block back to the driver
        /**
         * Drops all held handles -- their `device_handle` destructors call
         * `vkFreeMemory` -- and zeroes the driver-byte gauge.
         */
        void clear();


        /// ### Driver-byte and driver-block observability
        std::int64_t driver_bytes_in_use() const noexcept {
            return c_driver_bytes_in_use.value();
        }
        std::int64_t driver_bytes_peak() const noexcept {
            return static_cast<std::int64_t>(c_driver_bytes_peak.value());
        }
        std::size_t driver_blocks_allocated() const noexcept {
            return static_cast<std::size_t>(c_driver_blocks_allocated.value());
        }
        std::size_t driver_blocks_freed() const noexcept {
            return static_cast<std::size_t>(c_driver_blocks_freed.value());
        }


      private:
        /// ### Driver-byte and driver-block telemetry owned by the pool
        telemetry::counter c_driver_blocks_allocated, c_driver_blocks_freed,
                c_driver_bytes_in_use;
        telemetry::max c_driver_bytes_peak;
        telemetry::map<std::size_t, std::size_t> c_driver_block_sizes;
    };


}

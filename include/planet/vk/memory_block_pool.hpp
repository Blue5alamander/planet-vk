#pragma once


#include <planet/telemetry/counter.hpp>
#include <planet/telemetry/id.hpp>
#include <planet/telemetry/map.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/memory.hpp>

#include <cstdint>
#include <mutex>
#include <vector>


namespace planet::vk {


    /// ## Shared pool of whole driver memory blocks
    /**
     * A device-wide, thread-safe free list of whole driver allocations, one
     * free list per memory type. The pool serves a single block size (its
     * `driver_block_size`): allocators draw a recycled block of the right
     * memory type from here instead of pinning their own, and freed blocks
     * survive allocator teardown -- so a short-lived allocator no longer hands
     * its blocks back to the driver only for the next allocator to call
     * `vkAllocateMemory` again.
     *
     * This pool owns the driver-byte accounting: a block counts as in use from
     * the moment it is allocated from the driver until `clear` frees it,
     * whether it is currently checked out to an allocator or sitting in a free
     * list.
     */
    class device_memory_block_pool final : private telemetry::id {
        /// ### A free list of whole blocks for one memory type
        /**
         * Each free list owns its own `std::mutex`. The lists live in a
         * `std::vector` sized to the memory-type count at construction and
         * never resized, so a list's address -- and therefore its mutex -- is
         * stable for the pool's lifetime.
         */
        struct free_list final {
            std::mutex mtx;
            std::vector<device_memory_allocation::handle_type> blocks;
        };

        /// ### The one block size the pool retains
        /**
         * Only blocks of exactly this size are pooled. An `acquire` or
         * `release` for any other size -- larger (e.g. a swap-chain attachment,
         * which can be far larger than a driver block) or smaller -- goes
         * straight to the driver and is never retained, so the per-type free
         * lists only ever hold interchangeable, identically sized blocks.
         */
        std::size_t const driver_block_size;

        /// ### Free lists indexed by `memory_type_index`
        /**
         * Sized to the GPU's memory-type count at construction and never
         * resized. Indexing into it needs no lock; each `free_list::mtx` guards
         * only its own block vector.
         */
        std::vector<free_list> free_lists;

        free_list &list_for(std::uint32_t);


      public:
        /// ### Construct a pool
        /**
         * The instance supplies the GPU's memory-type count, which fixes the
         * number of free lists for the pool's lifetime. `driver_block_size` is
         * the one size the pool retains; any other size bypasses the free lists
         * entirely (see `driver_block_size`).
         *
         * The telemetry counters are named from the pool's type. The device
         * hosts a single pool and takes the default `suffix::suppress` for a
         * clean, stable name; anywhere several pools coexist (e.g. tests) pass
         * `suffix::add` so each gets a unique machine-generated suffix and the
         * counter names do not clash.
         */
        explicit device_memory_block_pool(
                vk::instance const &,
                std::size_t driver_block_size = 64u << 20,
                id::suffix = id::suffix::suppress);


        /// ### Acquire a whole block of the requested type and size
        /**
         * Pops a pooled block for `memory_type_index` when `block_size` matches
         * the pool's `driver_block_size` and one is free; otherwise allocates a
         * fresh one from the driver.
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
            return static_cast<std::int64_t>(
                    c_driver_bytes_in_use_peak.value());
        }
        std::size_t driver_blocks_allocated() const noexcept {
            return static_cast<std::size_t>(c_driver_blocks_allocated.value());
        }
        std::size_t driver_blocks_freed() const noexcept {
            return static_cast<std::size_t>(c_driver_blocks_freed.value());
        }


      private:
        /// ### Telemetry

        /// #### Fresh driver allocations performed by the pool
        telemetry::counter c_driver_blocks_allocated{
                name() + "__driver_blocks_allocated"};
        /**
         * Bumped by `acquire` whenever a free list misses and the pool calls
         * `vkAllocateMemory`. This is the authoritative driver-allocation count.
         */

        /// #### Driver blocks freed back to the driver
        telemetry::counter c_driver_blocks_freed{
                name() + "__driver_blocks_freed"};
        /**
         * Bumped when an oversized block is released, and on `clear` when every
         * pooled block's handle is dropped and `vkFreeMemory` runs.
         */

        /// #### Bytes currently held from the driver
        telemetry::counter c_driver_bytes_in_use{
                name() + "__driver_bytes_in_use"};
        telemetry::max c_driver_bytes_in_use_peak{
                name() + "__driver_bytes_peak"};
        /**
         * A block counts from the moment it is allocated from the driver until
         * `clear` frees it -- whether checked out to an allocator, idling in an
         * allocator's free list, or sitting in one of the pool's own free
         * lists. Unlike the allocator's `c_bytes_held`, this is the true
         * device-wide driver footprint.
         */

        /// #### Histogram of block sizes delivered by `acquire`
        telemetry::map<std::size_t, std::size_t> c_acquired_block_sizes{
                name() + "__acquired_block_sizes"};
        /**
         * Updated on every `acquire` -- free-list hit, driver miss, or
         * oversized bypass -- so it records the demand the pool serves.
         * `c_acquired_block_sizes` minus `c_driver_block_sizes` is the demand
         * met by free-list reuse.
         *
         * TODO This mirrors the allocator-side
         * `c_global_device_pool_block_sizes` (the allocator is `acquire`'s only
         * caller and records the same size just before calling in), so one of
         * the two is redundant -- decide which object should own it.
         */

        /// #### Histogram of block sizes freshly allocated from the driver
        telemetry::map<std::size_t, std::size_t> c_driver_block_sizes{
                name() + "__driver_block_sizes"};
        /**
         * The subset of `c_acquired_block_sizes` that missed the free list and
         * reached `vkAllocateMemory`.
         */
    };


}

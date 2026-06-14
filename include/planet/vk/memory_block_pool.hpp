#pragma once


#include <planet/telemetry/counter.hpp>
#include <planet/telemetry/id.hpp>
#include <planet/telemetry/map.hpp>
#include <planet/telemetry/minmax.hpp>
#include <planet/vk/memory.hpp>

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
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
     * This pool owns the driver-block accounting: a block counts as in use from
     * the moment it is allocated from the driver until `clear` frees it,
     * whether it is currently checked out to an allocator or sitting in a free
     * list.
     */
    class device_memory_block_pool final : private telemetry::id {
        /// ### A free list of whole blocks for one memory type
        /**
         * Each free list owns its own `std::mutex`, its own block vector, and
         * its own per-queue driver counters. The lists live in a `std::deque`
         * built to the memory-type count at construction and never grown past
         * it, so a list's address -- and therefore its mutex and counters -- is
         * stable for the pool's lifetime. A `std::deque` rather than a
         * `std::vector` because the counters are neither copyable nor movable,
         * so each list must be constructed in place.
         *
         * Each per-queue counter takes the matching pool-wide counter as its
         * `parent`, so every per-type change also rolls up into the device-wide
         * total. The pool-wide counter therefore stays the authoritative
         * aggregate the public getters read, while the per-queue counters give
         * the same numbers broken down by memory type in the telemetry dump.
         */
        struct free_list final {
            free_list(
                    std::string const &pool_name,
                    std::string const &memory_type_name,
                    telemetry::counter &blocks_allocated,
                    telemetry::counter &blocks_freed,
                    telemetry::counter &blocks_in_use);


            std::mutex mtx;
            std::vector<device_memory_allocation::handle_type> blocks;


            /// #### Per-queue driver block accounting
            /**
             * The peak is **not** parented: a `max` would present each queue's
             * own value to the pool-wide peak, but the pool's peak must track
             * the peak of the device-wide *total*, so the two peaks are each
             * set explicitly from their own in-use counter.
             */
            telemetry::counter c_driver_blocks_allocated, c_driver_blocks_freed,
                    c_driver_blocks_in_use;
            telemetry::max c_driver_blocks_in_use_peak;
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


      public:
        /// ### Construct a pool
        explicit device_memory_block_pool(
                vk::instance const &,
                std::size_t driver_block_size = 64u << 20,
                id::suffix = id::suffix::suppress);
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


        /// ### Acquire a whole block of the requested type and size
        device_memory_allocation::handle_type
                acquire(vk::device &,
                        std::uint32_t memory_type_index,
                        std::size_t block_size);
        /**
         * Pops a pooled block for `memory_type_index` when `block_size` matches
         * the pool's `driver_block_size` and one is free; otherwise allocates a
         * fresh one from the driver.
         */


        /// ### Return a whole block to its free list for later reuse
        void
                release(device_memory_allocation::handle_type,
                        std::uint32_t memory_type_index,
                        std::size_t block_size);

        /// ### Free every pooled block back to the driver
        void clear();
        /**
         * Drops all held handles -- their `device_handle` destructors call
         * `vkFreeMemory` -- and zeroes the driver-block gauge.
         */


        /// ### Driver-block observability
        std::size_t driver_blocks_in_use() const noexcept {
            return static_cast<std::size_t>(c_driver_blocks_in_use.value());
        }
        std::size_t driver_blocks_peak() const noexcept {
            return static_cast<std::size_t>(
                    c_driver_blocks_in_use_peak.value());
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

        /// #### Whole blocks currently held from the driver
        telemetry::counter c_driver_blocks_in_use{
                name() + "__driver_blocks_in_use"};
        telemetry::max c_driver_blocks_in_use_peak{
                name() + "__driver_blocks_peak"};
        /**
         * Counts a block from the moment it is allocated from the driver until
         * `clear` frees it -- whether checked out to an allocator, idling in an
         * allocator's free list, or sitting in one of the pool's own free
         * lists. This is the true count of whole driver allocations the device
         * is holding (equivalently `c_driver_blocks_allocated` minus
         * `c_driver_blocks_freed`), with `c_driver_blocks_in_use_peak` its
         * high-water mark.
         */

        /// #### Histogram of block sizes requested from `acquire`
        telemetry::map<std::size_t, std::size_t> c_requested_block_sizes{
                name() + "__requested_block_sizes"};
        /**
         * Updated on every `acquire` -- free-list hit, driver miss, or
         * oversized bypass -- so it records the demand the pool serves.
         * `c_requested_block_sizes` minus `c_driver_block_sizes` is the demand
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
         * The subset of `c_requested_block_sizes` that missed the free list and
         * reached `vkAllocateMemory`.
         */


        /// ### Free lists indexed by `memory_type_index`
        /**
         * One free list per memory type, built to the GPU's memory-type count
         * at construction (in the constructor body, once the pool-wide counters
         * above exist) and never grown past it. Indexing into it needs no lock;
         * each `free_list::mtx` guards only its own block vector.
         *
         * Declared **after** the pool-wide counters so that -- because members
         * destruct in reverse declaration order -- each free list, and the
         * per-queue counters that name those pool-wide counters as their
         * `parent`, is destroyed **before** the parents it points at.
         */
        std::deque<free_list> free_lists;
        free_list &list_for(std::uint32_t);
    };


}

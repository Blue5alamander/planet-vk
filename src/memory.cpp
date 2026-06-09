#include <planet/functional.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory.hpp>

#include <felspar/memory/sizes.hpp>


namespace {
    /// ### Telemetry

    /// #### Allocator instances ever constructed
    planet::telemetry::counter c_allocators_created{
            "planet_vk_device_memory_allocator__created"};

    /// #### Allocator instances ever destroyed
    planet::telemetry::counter c_allocators_destroyed{
            "planet_vk_device_memory_allocator__destroyed"};
    /** `created - destroyed` is the number of allocators currently live. */

    /// #### Raw `vkAllocateMemory` calls through `device_memory_allocation`
    planet::telemetry::counter c_allocations{
            "planet_vk_device_memory_allocator_memory__allocations"};
    /**
     * Bumped by `device_memory_allocation::allocate`, now reached only via
     * `device_memory_block_pool::acquire`, so it counts driver-level
     * allocations -- not calls to `device_memory_allocator::allocate`.
     *
     * TODO Misleading: the `..._allocator_memory__` scope attributes driver
     * allocations to the allocator, and this duplicates the pool's
     * `..._block_pool__..._driver_blocks_allocated`. Fold into the pool's
     * counter or rename to make the driver-call meaning explicit.
     */

    /// #### Raw `vkFreeMemory` calls through `device_memory_allocation`
    planet::telemetry::counter c_deallocations{
            "planet_vk_device_memory_allocator_memory__deallocations"};
    /**
     * TODO As with `c_allocations`: counts driver frees and duplicates the
     * pool's `..._driver_blocks_freed`, while reading like a count of
     * `device_memory_allocator` deallocations.
     */

    /// #### Cumulative bytes ever handed to `vkAllocateMemory`
    planet::telemetry::counter c_total_allocated{
            "planet_vk_device_memory_allocator_memory__total_allocated"};
    /**
     * Monotonic lifetime total of every driver allocation's size; never
     * decremented, so it is not a live figure.
     *
     * TODO Like `c_allocations`, this is driver-side accounting living on the
     * allocator scope, overlapping the pool's driver-byte counters. Consider
     * moving it onto `device_memory_block_pool`.
     */

    /// #### Histogram of requested allocation sizes across all allocators
    planet::telemetry::map<std::size_t, std::size_t> c_global_allocation_sizes{
            "planet_vk_device_memory_allocator_allocation__sizes"};
    /**
     * Keyed by the `bytes` asked of `device_memory_allocator::allocate` (before
     * block-size rounding); the value is the count of requests of that size.
     */

    /// #### Histogram of whole-block sizes pulled from the shared pool
    planet::telemetry::map<std::size_t, std::size_t>
            c_global_device_pool_block_sizes{
                    "planet_vk_device_memory_allocator_device_pool__block_sizes"};
    /**
     * Keyed by the block size acquired when an allocator misses its free list
     * and draws a fresh block from `device_memory_block_pool`.
     */

    /// #### Device-wide sum of every allocator's `c_bytes_held`
    planet::telemetry::counter c_global_bytes_held{
            "planet_vk_device_memory_allocator__bytes_held"};
    /**
     * Parent of each allocator's `c_bytes_held`: the total bytes checked out of
     * the shared pool across all live allocators. Trends upwards for the same
     * reason -- see `device_memory_allocator`.
     */

    /// #### Device-wide sum of every allocator's `c_free_blocks`
    planet::telemetry::counter c_global_free_blocks{
            "planet_vk_device_memory_allocator__free_blocks"};
    /** Whole blocks idling in allocator-local free lists, device-wide. */

    /// #### Peak of `c_global_bytes_held`
    planet::telemetry::max c_global_bytes_held_peak{
            "planet_vk_device_memory_allocator__bytes_held_peak"};
    /**
     * Driven from the global held total rather than the max of per-allocator
     * peaks, so it is the high-water mark of summed held bytes.
     */


    /// ### Function used to bump a telemetry counter
    auto const bump = [](auto &n) { ++n; };
}


/// ## `planet::vk::device_memory_allocation`


planet::vk::device_memory_allocation::handle_type
        planet::vk::device_memory_allocation::allocate(
                vk::device &device,
                std::size_t const bytes,
                std::uint32_t const memory_type_index) {
    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = bytes;
    alloc.memoryTypeIndex = memory_type_index;
    handle_type handle;
    handle.create<vkAllocateMemory>(device.get(), alloc, nullptr);
    ++c_allocations;
    c_total_allocated += bytes;
    return handle;
}

void planet::vk::device_memory_allocation::deallocate(
        VkDevice o, VkDeviceMemory a, const VkAllocationCallbacks *c) {
    vkFreeMemory(o, a, c);
    ++c_deallocations;
}


planet::vk::device_memory_allocation::device_memory_allocation(
        device_memory_allocator *a,
        handle_type h,
        std::uint32_t const mti,
        std::size_t const bytes)
: handle{std::move(h)},
  allocator{a},
  memory_type_index{mti},
  allocation_size{bytes} {
    if (allocator) { ++allocator->c_memory_allocation_count; }
}

planet::vk::device_memory_allocation::~device_memory_allocation() {
    if (allocator) {
        allocator->deallocate(
                std::move(handle), memory_type_index, allocation_size);
    }
}


void planet::vk::device_memory_allocation::decrement(
        device_memory_allocation *&m) noexcept {
    device_memory_allocation *alloc = std::exchange(m, nullptr);
    if (alloc) {
        ++alloc->allocator->c_memory_deallocation_count;
        if (--alloc->ownership_count == 0u) { delete alloc; }
    }
}


planet::vk::device_memory_allocation *
        planet::vk::device_memory_allocation::increment(
                device_memory_allocation *alloc) noexcept {
    if (alloc) {
        ++alloc->allocator->c_memory_allocation_count;
        ++alloc->ownership_count;
    }
    return alloc;
}


std::byte *planet::vk::device_memory_allocation::map_memory() {
    std::scoped_lock _{mapping_mtx};
    if (not mapping_count) {
        void *p = nullptr;
        worked(vkMapMemory(
                handle.owner(), handle.get(), {}, allocation_size,
                allocator->config.memory_map_flags, &p));
        mapped_base = reinterpret_cast<std::byte *>(p);
    }
    ++mapping_count;
    return mapped_base;
}


void planet::vk::device_memory_allocation::unmap_memory() {
    std::scoped_lock _{mapping_mtx};
    if (--mapping_count == 0) { vkUnmapMemory(handle.owner(), handle.get()); }
}


/// ## `planet::vk::device_memory_allocator`


planet::vk::device_memory_allocator::device_memory_allocator(
        std::string_view const n,
        vk::device &d,
        device_memory_allocator_configuration const &c,
        id::suffix const s)
: id{"planet_vk_device_memory_allocator__" + std::string{n}, s},
  pools(d.instance.gpu().memory_properties.memoryTypeCount),
  device{d},
  config{c},
  // The remaining counters are constructed via NSDMI in the header; these two
  // stay here because their `c_global_*` parents are file-local to this `.cpp`.
  c_bytes_held{name() + "__bytes_held", c_global_bytes_held},
  c_free_blocks{name() + "__free_blocks", c_global_free_blocks} {
    ++c_allocators_created;
}

planet::vk::device_memory_allocator::~device_memory_allocator() {
    clear_without_check();
    ++c_allocators_destroyed;
}


planet::vk::device_memory planet::vk::device_memory_allocator::allocate(
        std::size_t const bytes_requested,
        std::uint32_t const memory_type_index,
        std::size_t const alignment) {
    c_allocation_sizes.update(bytes_requested, 1u, bump);
    c_global_allocation_sizes.update(bytes_requested, 1u, bump);
    auto const bytes = felspar::memory::block_size(bytes_requested, alignment);
    auto &pool = pools[memory_type_index];
    std::scoped_lock _{pool.mtx};
    /**
     * The current splitting block must have room for `bytes` *after* its offset
     * is realigned to `alignment` -- otherwise carve from a fresh block. A
     * bare `size() < bytes` check would miss the alignment padding and let
     * `split` throw on a request a fresh block could satisfy.
     */
    if (not pool.splitting.can_split(bytes, alignment)) {
        pool.splitting.reset();
        auto const allocating = std::max(bytes, config.allocation_block_size);
        device_memory_allocation::handle_type handle;
        if (allocating > config.allocation_block_size
            or pool.free_memory.empty()) {
            ++c_block_allocation_from_device_pool;
            c_device_pool_block_sizes.update(allocating, 1u, bump);
            c_global_device_pool_block_sizes.update(allocating, 1u, bump);
            c_bytes_held += static_cast<std::int64_t>(allocating);
            c_bytes_held_peak.value(
                    static_cast<std::uint64_t>(c_bytes_held.value()));
            c_global_bytes_held_peak.value(
                    static_cast<std::uint64_t>(c_global_bytes_held.value()));
            handle = device().block_pool.acquire(
                    device, memory_type_index, allocating);
        } else {
            handle = std::move(pool.free_memory.back());
            pool.free_memory.pop_back();
            --c_free_blocks;
            ++c_block_allocation_from_free_list;
        }
        pool.splitting = {
                new device_memory_allocation(
                        this, std::move(handle), memory_type_index, allocating),
                {},
                allocating};
    }
    if (config.split) {
        return pool.splitting.split(bytes, alignment);
    } else {
        return std::exchange(pool.splitting, {});
    }
}
planet::vk::device_memory planet::vk::device_memory_allocator::allocate(
        VkMemoryRequirements const requirements,
        VkMemoryPropertyFlags const flags) {
    return allocate(
            requirements.size,
            device().instance.find_memory_type(requirements, flags),
            requirements.alignment);
}


void planet::vk::device_memory_allocator::deallocate(
        device_memory_allocation::handle_type h,
        std::uint32_t const memory_type_index,
        std::size_t const size) {
    if (size <= config.allocation_block_size) {
        auto &pool = pools[memory_type_index];
        std::scoped_lock _{pool.mtx};
        pool.free_memory.push_back(std::move(h));
        ++c_free_blocks;
        ++c_block_deallocated_added_to_free_list;
    } else {
        ++c_block_deallocation_returned_to_device_pool;
        c_bytes_held -= static_cast<std::int64_t>(size);
        device().block_pool.release(std::move(h), memory_type_index, size);
    }
}


void planet::vk::device_memory_allocator::clear_without_check() {
    planet::by_index(pools, [&](std::size_t const memory_type_index, pool &p) {
        /**
         * Return the block currently being split from to the free list. Its
         * bytes were added to `c_bytes_in_use` when it was pulled from the
         * pool, so without this the block would neither be handed back below
         * nor freed while the device is still alive.
         */
        /**
         * Funnel the block currently being split from back into the local free
         * list: `reset` drops the last reference, whose
         * `~device_memory_allocation` routes the whole block through
         * `deallocate` into `free_memory` (or, if oversized, straight back to
         * the pool). The drain below then hands it to the shared pool along
         * with the rest. This is why the reset must run before the drain.
         */
        p.splitting.reset();
        /**
         * Hand every whole block in the local free list back to the shared
         * device pool rather than dropping it. Dropping would free the block to
         * the driver behind the pool's back -- leaving the pool's driver-byte
         * accounting overcounted and the block lost for reuse by the next
         * allocator. Every pooled block is exactly `allocation_block_size`.
         */
        for (auto &&handle : p.free_memory) {
            device().block_pool.release(
                    std::move(handle),
                    static_cast<std::uint32_t>(memory_type_index),
                    config.allocation_block_size);
        }
        c_bytes_held -= static_cast<std::int64_t>(
                p.free_memory.size() * config.allocation_block_size);
        c_free_blocks -= static_cast<std::int64_t>(p.free_memory.size());
    });
    pools.clear();
}


/// ## `planet::vk::device_memory`


planet::vk::device_memory::device_memory(device_memory &&m)
: allocation{std::exchange(m.allocation, nullptr)},
  offset{std::exchange(m.offset, {})},
  byte_count{std::exchange(m.byte_count, {})} {}


planet::vk::device_memory &
        planet::vk::device_memory::operator=(device_memory &&m) {
    reset();
    allocation = std::exchange(m.allocation, nullptr);
    offset = std::exchange(m.offset, {});
    byte_count = std::exchange(m.byte_count, {});
    return *this;
}


void planet::vk::device_memory::bind_buffer_memory(VkBuffer const buffer_handle) {
    worked(vkBindBufferMemory(
            allocation->handle.owner(), buffer_handle, get(), offset));
}
void planet::vk::device_memory::bind_image_memory(VkImage const image_handle) {
    worked(vkBindImageMemory(
            allocation->handle.owner(), image_handle, get(), offset));
}


planet::vk::device_memory::mapping planet::vk::device_memory::map_memory(
        VkDeviceSize const extra_offset, VkDeviceSize const size) {
    return {allocation, offset + extra_offset, size};
}


bool planet::vk::device_memory::can_split(
        std::size_t const bytes, std::size_t const alignment) const noexcept {
    auto const aligned_offset =
            felspar::memory::aligned_offset(offset, alignment);
    return bytes + aligned_offset - offset <= byte_count;
}


planet::vk::device_memory planet::vk::device_memory::split(
        std::size_t const bytes, std::size_t const alignment) {
    if (not can_split(bytes, alignment)) {
        throw felspar::stdexcept::logic_error{
                "The split is larger than the memory block"};
    }
    auto aligned_offset = felspar::memory::aligned_offset(offset, alignment);
    auto const growth = bytes + aligned_offset - offset;
    device_memory first{
            device_memory_allocation::increment(allocation), aligned_offset,
            bytes};
    byte_count -= growth;
    offset += growth;
    return first;
}


/// ## `planet::vk::device_memory::mapping`


planet::vk::device_memory::mapping::mapping(
        device_memory_allocation *const a,
        VkDeviceSize const offset,
        VkDeviceSize)
: allocation{a}, pointer{[&]() {
      std::byte *base = allocation->map_memory();
      return base + offset;
  }()} {}


planet::vk::device_memory::mapping::mapping(mapping &&m)
: allocation{std::exchange(m.allocation, nullptr)},
  pointer{std::exchange(m.pointer, nullptr)} {}


planet::vk::device_memory::mapping &
        planet::vk::device_memory::mapping::operator=(
                planet::vk::device_memory::mapping &&m) {
    unsafe_reset();
    allocation = std::exchange(m.allocation, nullptr);
    pointer = std::exchange(m.pointer, nullptr);
    return *this;
}


void planet::vk::device_memory::mapping::unsafe_reset() {
    /// Note that this does not reset the internal fields
    if (allocation) { allocation->unmap_memory(); }
}

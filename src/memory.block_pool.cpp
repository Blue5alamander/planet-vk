#include <planet/functional.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory_block_pool.hpp>


/// ## `planet::vk::device_memory_block_pool`


namespace {
    auto const bump = [](auto &n) { ++n; };
}


planet::vk::device_memory_block_pool::device_memory_block_pool(
        vk::instance const &instance,
        std::size_t const driver_block_size,
        id::suffix const s)
: id{"planet_vk_device_memory_block_pool", s},
  driver_block_size{driver_block_size} {
    planet::by_index(
            instance.gpu().memory_properties.memoryTypeCount,
            [&](std::size_t const index) {
                free_lists.emplace_back(
                        name(),
                        instance.gpu().memory_type_name(
                                static_cast<std::uint32_t>(index)),
                        c_driver_blocks_allocated, c_driver_blocks_freed,
                        c_driver_blocks_in_use);
            });
}


planet::vk::device_memory_block_pool::free_list &
        planet::vk::device_memory_block_pool::list_for(
                std::uint32_t const memory_type_index) {
    return free_lists.at(memory_type_index);
}


planet::vk::device_memory_allocation::handle_type
        planet::vk::device_memory_block_pool::acquire(
                vk::device &device,
                std::uint32_t const memory_type_index,
                std::size_t const block_size) {
    c_requested_block_sizes.update(block_size, 1u, bump);
    auto &list = list_for(memory_type_index);
    /**
     * Only blocks of exactly `driver_block_size` are pooled, so any other size
     * skips the free-list lookup and goes straight to the driver. A matching
     * size checks its free list first.
     */
    if (block_size == driver_block_size) {
        std::scoped_lock _{list.mtx};
        if (not list.blocks.empty()) {
            auto handle = std::move(list.blocks.back());
            list.blocks.pop_back();
            return handle;
        }
    }
    auto handle = device_memory_allocation::allocate(
            device, block_size, memory_type_index);
    ++list.c_driver_blocks_allocated;
    c_driver_block_sizes.update(block_size, 1u, bump);
    ++list.c_driver_blocks_in_use;
    list.c_driver_blocks_in_use_peak.value(
            static_cast<std::uint64_t>(list.c_driver_blocks_in_use.value()));
    c_driver_blocks_in_use_peak.value(
            static_cast<std::uint64_t>(c_driver_blocks_in_use.value()));
    return handle;
}


void planet::vk::device_memory_block_pool::release(
        device_memory_allocation::handle_type handle,
        std::uint32_t const memory_type_index,
        std::size_t const block_size) {
    auto &list = list_for(memory_type_index);
    if (block_size != driver_block_size) {
        /**
         * Only blocks of exactly `driver_block_size` are retained: free any
         * other size straight back to the driver and account for it here,
         * mirroring the acquire bypass. (Poolable blocks stay in their free
         * list and are only counted out at `clear`.) `handle` is freed by its
         * destructor as it leaves this scope.
         */
        ++list.c_driver_blocks_freed;
        --list.c_driver_blocks_in_use;
        return;
    }
    std::scoped_lock _{list.mtx};
    list.blocks.push_back(std::move(handle));
}


void planet::vk::device_memory_block_pool::clear() {
    for (auto &&list : free_lists) {
        std::scoped_lock _l{list.mtx};
        auto const freed = static_cast<std::int64_t>(list.blocks.size());
        list.c_driver_blocks_freed += freed;
        list.blocks.clear();
        /// Drop the freed blocks from the in-use gauge; the `parent` follows.
        list.c_driver_blocks_in_use -= freed;
    }
}


/// ## `planet::vk::device_memory_block_pool::free_list`


namespace {
    /// Build a per-queue counter name like
    /// `<pool>__2:DEVICE_LOCAL|HOST_VISIBLE@heap0__<leaf>`
    std::string queue_counter_name(
            std::string const &pool,
            std::string const &memory_type_name,
            std::string_view const leaf) {
        return pool + "__" + memory_type_name + "__" + std::string{leaf};
    }
}


planet::vk::device_memory_block_pool::free_list::free_list(
        std::string const &pool_name,
        std::string const &memory_type_name,
        telemetry::counter &blocks_allocated,
        telemetry::counter &blocks_freed,
        telemetry::counter &blocks_in_use)
: c_driver_blocks_allocated{queue_counter_name(pool_name, memory_type_name, "driver_blocks_allocated"), blocks_allocated},
  c_driver_blocks_freed{
          queue_counter_name(pool_name, memory_type_name, "driver_blocks_freed"),
          blocks_freed},
  c_driver_blocks_in_use{
          queue_counter_name(
                  pool_name, memory_type_name, "driver_blocks_in_use"),
          blocks_in_use},
  c_driver_blocks_in_use_peak{queue_counter_name(
          pool_name, memory_type_name, "driver_blocks_peak")} {}

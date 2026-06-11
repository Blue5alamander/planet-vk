#include <planet/vk/instance.hpp>
#include <planet/vk/memory_block_pool.hpp>


namespace {
    auto const bump = [](auto &n) { ++n; };
}


planet::vk::device_memory_block_pool::device_memory_block_pool(
        vk::instance const &instance,
        std::size_t const driver_block_size,
        id::suffix const s)
: id{"planet_vk_device_memory_block_pool", s},
  driver_block_size{driver_block_size},
  free_lists(instance.gpu().memory_properties.memoryTypeCount) {}


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
    c_acquired_block_sizes.update(block_size, 1u, bump);
    /**
     * Only blocks of exactly `driver_block_size` are pooled, so any other size
     * skips the free-list lookup and goes straight to the driver. A matching
     * size checks its free list first.
     */
    if (block_size == driver_block_size) {
        auto &list = list_for(memory_type_index);
        std::scoped_lock _{list.mtx};
        if (not list.blocks.empty()) {
            auto handle = std::move(list.blocks.back());
            list.blocks.pop_back();
            return handle;
        }
    }
    auto handle = device_memory_allocation::allocate(
            device, block_size, memory_type_index);
    ++c_driver_blocks_allocated;
    c_driver_block_sizes.update(block_size, 1u, bump);
    c_driver_bytes_in_use += static_cast<std::int64_t>(block_size);
    c_driver_bytes_in_use_peak.value(
            static_cast<std::uint64_t>(c_driver_bytes_in_use.value()));
    return handle;
}


void planet::vk::device_memory_block_pool::release(
        device_memory_allocation::handle_type handle,
        std::uint32_t const memory_type_index,
        std::size_t const block_size) {
    if (block_size != driver_block_size) {
        /**
         * Only blocks of exactly `driver_block_size` are retained: free any
         * other size straight back to the driver and account for it here,
         * mirroring the acquire bypass. (Poolable blocks stay in their free
         * list and are only counted out at `clear`.) `handle` is freed by its
         * destructor as it leaves this scope.
         */
        ++c_driver_blocks_freed;
        c_driver_bytes_in_use -= static_cast<std::int64_t>(block_size);
        return;
    }
    auto &list = list_for(memory_type_index);
    std::scoped_lock _{list.mtx};
    list.blocks.push_back(std::move(handle));
}


void planet::vk::device_memory_block_pool::clear() {
    for (auto &&list : free_lists) {
        std::scoped_lock _l{list.mtx};
        c_driver_blocks_freed += static_cast<std::int64_t>(list.blocks.size());
        list.blocks.clear();
    }
    c_driver_bytes_in_use.value(0);
}

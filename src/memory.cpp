#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory.hpp>

#include <felspar/memory/sizes.hpp>


namespace {
    planet::telemetry::counter c_allocators_created{
            "planet_vk_device_memory_allocator_created"};
    planet::telemetry::counter c_allocators_destroyed{
            "planet_vk_device_memory_allocator_destroyed"};
    planet::telemetry::counter c_allocations{
            "planet_vk_device_memory_allocator_memory_allocations"};
    planet::telemetry::counter c_deallocations{
            "planet_vk_device_memory_allocator_memory_deallocations"};
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
    handle.create<vkAllocateMemory>(
            device.get(), alloc, nullptr, &c_deallocations);
    ++c_allocations;
    return handle;
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
        vk::device &d, device_memory_allocator_configuration const &c)
: device_memory_allocator{
          "planet_vk_device_memory_allocator", d, c, id::suffix::yes} {}

planet::vk::device_memory_allocator::device_memory_allocator(
        std::string_view const n,
        vk::device &d,
        device_memory_allocator_configuration const &c,
        id::suffix const s)
: id{"planet_vk_device_memory_allocator_" + std::string{n}, s},
  pools(d.instance.gpu().memory_properties.memoryTypeCount),
  device{d},
  config{c},
  c_block_allocation_from_driver{name() + "_block_allocation_from_driver"},
  c_block_allocation_from_free_list{
          name() + "_block_allocation_from_free_list"},
  c_block_deallocated_added_to_free_list{
          name() + "_block_deallocated_added_to_free_list"},
  c_block_deallocation_returned_to_driver{
          name() + "_block_deallocation_returned_to_driver"},
  c_memory_allocation_count{name() + "_memory_allocation_count"},
  c_memory_deallocation_count{name() + "_memory_deallocation_count"} {
    ++c_allocators_created;
}

planet::vk::device_memory_allocator::~device_memory_allocator() {
    ++c_allocators_destroyed;
}


planet::vk::device_memory planet::vk::device_memory_allocator::allocate(
        std::size_t const bytes_requested,
        std::uint32_t const memory_type_index,
        std::size_t const alignment) {
    auto const bytes = felspar::memory::block_size(bytes_requested, alignment);
    auto &pool = pools[memory_type_index];
    std::scoped_lock _{pool.mtx};
    if (pool.splitting.size() < bytes) {
        pool.splitting.reset();
        auto const allocating = std::max(bytes, config.allocation_block_size);
        device_memory_allocation::handle_type handle;
        if (allocating > config.allocation_block_size
            or pool.free_memory.empty()) {
            ++c_block_allocation_from_driver;
            handle = device_memory_allocation::allocate(
                    device, allocating, memory_type_index);
        } else {
            handle = std::move(pool.free_memory.back());
            pool.free_memory.pop_back();
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
        ++c_block_deallocated_added_to_free_list;
    } else {
        ++c_block_deallocation_returned_to_driver;
    }
}


void planet::vk::device_memory_allocator::clear_without_check() {
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


planet::vk::device_memory planet::vk::device_memory::split(
        std::size_t const bytes, std::size_t const alignment) {
    auto aligned_offset = felspar::memory::aligned_offset(offset, alignment);
    auto const growth = bytes + aligned_offset - offset;
    if (growth > byte_count) {
        throw felspar::stdexcept::logic_error{
                "The split is larger than the memory block"};
    }
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

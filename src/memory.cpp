#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory.hpp>

#include <felspar/memory/sizes.hpp>


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
    handle.create<vkAllocateMemory>(device.get(), alloc);
    return handle;
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
    if (alloc and --alloc->ownership_count == 0u) { delete alloc; }
}


planet::vk::device_memory_allocation *
        planet::vk::device_memory_allocation::increment(
                device_memory_allocation *alloc) noexcept {
    if (alloc) { ++alloc->ownership_count; }
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
: pools(d.instance.gpu().memory_properties.memoryTypeCount),
  device{d},
  config{c} {}


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
            handle = device_memory_allocation::allocate(
                    device, allocating, memory_type_index);
        } else {
            handle = std::move(pool.free_memory.back());
            pool.free_memory.pop_back();
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

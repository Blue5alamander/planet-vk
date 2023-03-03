#include <planet/vk/device.hpp>
#include <planet/vk/memory.hpp>


/// ## `planet::vk::device_memory_allocation`


planet::vk::device_memory_allocation::handle_type
        planet::vk::device_memory_allocation::allocate(
                vk::device const &device,
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
    if (allocator) { allocator->deallocate(std::move(handle)); }
}


void planet::vk::device_memory_allocation::decrement(
        device_memory_allocation *&m) noexcept {
    device_memory_allocation *alloc = std::exchange(m, nullptr);
    if (alloc and --alloc->ownership_count == 0u) { delete alloc; }
}


/// ## `planet::vk::device_memory_allocator`


planet::vk::device_memory planet::vk::device_memory_allocator::allocate(
        std::size_t const bytes, std::uint32_t const memory_type_index) {
    return new device_memory_allocation(
            this,
            device_memory_allocation::allocate(
                    device, bytes, memory_type_index));
}


void planet::vk::device_memory_allocator::deallocate(
        device_memory_allocation::handle_type) {}


/// ## `planet::vk::device_memory`


planet::vk::device_memory::mapping planet::vk::device_memory::map_memory(
        VkDeviceSize const offset,
        VkDeviceSize const size,
        VkMemoryMapFlags flags) {
    return {allocation->handle.owner(), allocation->handle.get(), offset, size,
            flags};
}


planet::vk::device_memory &
        planet::vk::device_memory::operator=(device_memory &&m) {
    reset();
    allocation = std::exchange(m.allocation, nullptr);
    return *this;
}


/// ## `planet::vk::device_memory::mapping`


planet::vk::device_memory::mapping::mapping()
: device_handle{}, memory_handle{} {}


planet::vk::device_memory::mapping::mapping(
        VkDevice const d,
        VkDeviceMemory const m,
        VkDeviceSize const offset,
        VkDeviceSize const size,
        VkMemoryMapFlags flags)
: device_handle{d}, memory_handle{m}, pointer{[&]() {
      void *p = nullptr;
      worked(vkMapMemory(device_handle, memory_handle, offset, size, flags, &p));
      return p;
  }()} {}


planet::vk::device_memory::mapping::mapping(mapping &&m)
: device_handle{std::exchange(m.device_handle, {})},
  memory_handle{std::exchange(m.memory_handle, {})},
  pointer{std::exchange(m.pointer, nullptr)} {}


void planet::vk::device_memory::mapping::unsafe_reset() {
    /// Note that this does not reset the internal fields
    if (pointer) { vkUnmapMemory(device_handle, memory_handle); }
}


planet::vk::device_memory::mapping &
        planet::vk::device_memory::mapping::operator=(
                planet::vk::device_memory::mapping &&m) {
    unsafe_reset();
    device_handle = std::exchange(m.device_handle, {});
    memory_handle = std::exchange(m.memory_handle, {});
    pointer = std::exchange(m.pointer, nullptr);
    return *this;
}

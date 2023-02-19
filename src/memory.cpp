#include <planet/vk/device.hpp>
#include <planet/vk/memory.hpp>


/// ## `planet::vk::device_memory`


planet::vk::device_memory::device_memory(
        vk::device const &device,
        std::size_t const bytes,
        std::uint32_t const memory_type_index) {
    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = bytes;
    alloc.memoryTypeIndex = memory_type_index;
    handle.create<vkAllocateMemory>(device.get(), alloc);
}


planet::vk::device_memory::mapping planet::vk::device_memory::map_memory(
        VkDeviceSize const offset,
        VkDeviceSize const size,
        VkMemoryMapFlags flags) {
    return {handle.owner(), handle.get(), offset, size, flags};
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

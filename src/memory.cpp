#include <planet/vk/device.hpp>
#include <planet/vk/memory.hpp>


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

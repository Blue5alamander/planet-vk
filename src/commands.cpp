#include <planet/vk/commands.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/surface.hpp>


/**
 * ## planet::vk::command_pool
 */


planet::vk::command_pool::command_pool(vk::device const &d, vk::surface const &s)
: device{d} {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = s.graphics_queue_index();
    handle.create<vkCreateCommandPool>(device.get(), info);
}


/**
 * ## planet::vk::command_buffers
 */


planet::vk::command_buffers::command_buffers(
        vk::command_pool const &cp,
        std::size_t const count,
        VkCommandBufferLevel const level)
: device{cp.device}, command_pool{cp} {
    handles.resize(count, VK_NULL_HANDLE);
    buffers.reserve(count);

    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = command_pool.get();
    info.level = level;
    info.commandBufferCount = handles.size();
    planet::vk::worked(
            vkAllocateCommandBuffers(device.get(), &info, handles.data()));

    for (auto const h : handles) {
        buffers.push_back(command_buffer{command_pool, h});
    }
}


planet::vk::command_buffers::~command_buffers() {
    vkFreeCommandBuffers(
            device.get(), command_pool.get(), handles.size(), handles.data());
}


/**
 * ## planet::vk::command_buffer
 */


planet::vk::command_buffer::command_buffer(
        vk::command_pool const &cp, VkCommandBufferLevel const level)
: self_owned{true}, device{cp.device}, command_pool{cp} {
    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.level = level;
    info.commandPool = command_pool.get();
    info.commandBufferCount = 1;

    worked(vkAllocateCommandBuffers(device.get(), &info, &handle));
}


planet::vk::command_buffer::command_buffer(
        vk::command_pool const &cp, VkCommandBuffer const h)
: handle{h}, device{cp.device}, command_pool{cp} {}


planet::vk::command_buffer::command_buffer(command_buffer &&b)
: handle{std::exchange(b.handle, VK_NULL_HANDLE)},
  device{b.device},
  command_pool{b.command_pool} {}


planet::vk::command_buffer::~command_buffer() {
    if (handle and self_owned) {
        std::array handles{handle};
        vkFreeCommandBuffers(
                device.get(), command_pool.get(), handles.size(),
                handles.data());
    }
}


void planet::vk::command_buffer::begin(VkCommandBufferUsageFlags const flags) {
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = flags;
    worked(vkBeginCommandBuffer(handle, &info));
}


void planet::vk::command_buffer::end() { worked(vkEndCommandBuffer(handle)); }


void planet::vk::command_buffer::submit(VkQueue const queue) {
    std::array buffers{handle};
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = buffers.size();
    info.pCommandBuffers = buffers.data();
    vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE);
}

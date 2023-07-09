#include <planet/vk/commands.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/surface.hpp>


/// ## `planet::vk::command_pool`


planet::vk::command_pool::command_pool(vk::device &d, vk::surface const &s)
: device{d} {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = s.graphics_queue_index();
    handle.create<vkCreateCommandPool>(device.get(), info);
}


/// ## `planet::vk::command_buffers`


planet::vk::command_buffers::command_buffers(
        vk::command_pool &cp,
        std::size_t const count,
        VkCommandBufferLevel const level)
: device{cp.device}, command_pool{cp} {
    handles.resize(count, VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = command_pool.get();
    info.level = level;
    info.commandBufferCount = handles.size();
    planet::vk::worked(
            vkAllocateCommandBuffers(device.get(), &info, handles.data()));

    buffers.reserve(count);
    for (auto const h : handles) {
        buffers.push_back(command_buffer{command_pool, h});
    }
}


planet::vk::command_buffers::~command_buffers() {
    vkFreeCommandBuffers(
            device.get(), command_pool.get(), handles.size(), handles.data());
}


/// ## `planet::vk::command_buffer`


planet::vk::command_buffer::command_buffer(
        vk::command_pool &cp, VkCommandBufferLevel const level)
: self_owned{true}, device{cp.device}, command_pool{cp} {
    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.level = level;
    info.commandPool = command_pool.get();
    info.commandBufferCount = 1;

    worked(vkAllocateCommandBuffers(device.get(), &info, &handle));
}


planet::vk::command_buffer::command_buffer(
        vk::command_pool &cp, VkCommandBuffer const h)
: handle{h}, device{cp.device}, command_pool{cp} {}


planet::vk::command_buffer::command_buffer(command_buffer &&b)
: handle{std::exchange(b.handle, VK_NULL_HANDLE)},
  device{std::move(b.device)},
  command_pool{std::move(b.command_pool)} {}


auto planet::vk::command_buffer::operator=(command_buffer &&cb)
        -> command_buffer & {
    reset();
    handle = std::exchange(cb.handle, VK_NULL_HANDLE);
    device = std::move(cb.device);
    command_pool = std::move(cb.command_pool);
    return *this;
}


void planet::vk::command_buffer::reset() {
    if (handle and self_owned) {
        std::array handles{handle};
        vkFreeCommandBuffers(
                device.get(), command_pool.get(), handles.size(),
                handles.data());
    }
}


planet::vk::command_buffer
        planet::vk::command_buffer::single_use(vk::command_pool &cp) {
    planet::vk::command_buffer cb{cp};
    cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return cb;
}
void planet::vk::command_buffer::end_and_submit() {
    end();
    submit(device().graphics_queue);
    vkQueueWaitIdle(device().graphics_queue);
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

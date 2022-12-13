#include <planet/vk/device.hpp>
#include <planet/vk/synchronisation.hpp>


/**
 * planet::vk::fence
 */


planet::vk::fence::fence(vk::device const &d) : device{d} {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    handle.create<vkCreateFence>(device.get(), info);
}


/**
 * planet::vk::semaphore
 */


planet::vk::semaphore::semaphore(vk::device const &d) : device{d} {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    handle.create<vkCreateSemaphore>(device.get(), info);
}

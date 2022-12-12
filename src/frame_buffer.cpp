#include <planet/vk/device.hpp>
#include <planet/vk/frame_buffer.hpp>


/**
 * planet::vk::frame_buffer
 */


planet::vk::frame_buffer::frame_buffer(
        vk::device const &d, VkFramebufferCreateInfo const &info)
: device{d} {
    handle.create<vkCreateFramebuffer>(device.get(), info);
}

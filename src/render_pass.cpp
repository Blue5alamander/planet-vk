#include <planet/vk/device.hpp>
#include <planet/vk/render_pass.hpp>


/// ## `planet::vk::render_pass`


planet::vk::render_pass::render_pass(
        vk::device const &d, VkRenderPassCreateInfo const &info)
: device{d} {
    handle.create<vkCreateRenderPass>(device.get(), info);
}

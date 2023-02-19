#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>


/// ## `planet::vk::descriptor_set_layout`


planet::vk::descriptor_set_layout::descriptor_set_layout(
        vk::device const &d, VkDescriptorSetLayoutBinding const &layout)
: device{d} {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &layout;

    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}

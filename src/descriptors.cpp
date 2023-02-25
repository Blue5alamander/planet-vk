#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>


/// ## `planet::vk::descriptor_pool`


planet::vk::descriptor_pool::descriptor_pool(
        vk::device const &device,
        std::span<VkDescriptorPoolSize const> sizes,
        std::uint32_t const max) {
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = sizes.size();
    info.pPoolSizes = sizes.data();
    info.maxSets = max;

    handle.create<vkCreateDescriptorPool>(device.get(), info);
}


planet::vk::descriptor_pool::descriptor_pool(
        vk::device const &device, std::uint32_t const count) {
    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    size.descriptorCount = count;
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &size;
    info.maxSets = count;

    handle.create<vkCreateDescriptorPool>(device.get(), info);
}


/// ## `planet::vk::descriptor_set_layout`


planet::vk::descriptor_set_layout::descriptor_set_layout(
        vk::device const &device, VkDescriptorSetLayoutCreateInfo const &info) {
    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}


planet::vk::descriptor_set_layout::descriptor_set_layout(
        vk::device const &device, VkDescriptorSetLayoutBinding const &layout) {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &layout;

    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}

#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>


/// ## `planet::vk::descriptor_pool`


planet::vk::descriptor_pool::descriptor_pool(
        vk::device const &d,
        std::span<VkDescriptorPoolSize const> sizes,
        std::uint32_t const max)
: device{d} {
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = sizes.size();
    info.pPoolSizes = sizes.data();
    info.maxSets = max;

    handle.create<vkCreateDescriptorPool>(device.get(), info);
}


planet::vk::descriptor_pool::descriptor_pool(
        vk::device const &d, std::uint32_t const count)
: device{d} {
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
        vk::device const &d, VkDescriptorSetLayoutCreateInfo const &info)
: device{d} {
    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}


planet::vk::descriptor_set_layout::descriptor_set_layout(
        vk::device const &d, VkDescriptorSetLayoutBinding const &layout)
: device{d} {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &layout;

    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}


/// ## `planet::vk::descriptor_sets`


planet::vk::descriptor_sets::descriptor_sets(
        descriptor_pool const &pool,
        descriptor_set_layout const &layout,
        std::uint32_t const count)
: sets(count) {
    std::vector<VkDescriptorSetLayout> layouts(count, layout.get());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool.get();
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    planet::vk::worked(vkAllocateDescriptorSets(
            pool.device.get(), &allocInfo, sets.data()));
}

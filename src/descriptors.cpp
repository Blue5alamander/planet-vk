#include <planet/vk/descriptors.hpp>
#include <planet/vk/device.hpp>


/// ## `planet::vk::descriptor_pool`


planet::vk::descriptor_pool::descriptor_pool(
        vk::device &d,
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
        vk::device &d, VkDescriptorType const type, std::uint32_t const count)
: device{d} {
    VkDescriptorPoolSize size{};
    size.type = type;
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
        vk::device &d, VkDescriptorSetLayoutCreateInfo const &info)
: device{d} {
    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}


planet::vk::descriptor_set_layout::descriptor_set_layout(
        vk::device &d, VkDescriptorSetLayoutBinding const &layout)
: device{d} {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &layout;

    handle.create<vkCreateDescriptorSetLayout>(device.get(), info);
}


planet::vk::descriptor_set_layout
        planet::vk::descriptor_set_layout::for_uniform_buffer_object(
                vk::device &device) {
    VkDescriptorSetLayoutBinding ubo_binding{};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    return vk::descriptor_set_layout{device, ubo_binding};
}


/// ## `planet::vk::descriptor_sets`


planet::vk::descriptor_sets::descriptor_sets(
        descriptor_pool const &pool,
        descriptor_set_layout const &layout,
        std::uint32_t const count,
        felspar::source_location const &loc)
: sets(count) {
    std::vector<VkDescriptorSetLayout> layouts(count, layout.get());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool.get();
    allocInfo.descriptorSetCount = layouts.size();
    allocInfo.pSetLayouts = layouts.data();

    planet::vk::worked(
            vkAllocateDescriptorSets(pool.device.get(), &allocInfo, sets.data()),
            loc);
}

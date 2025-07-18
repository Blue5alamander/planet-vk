#pragma once


#include <planet/vk/buffer.hpp>
#include <planet/vk/descriptors.hpp>


namespace planet::vk::engine {


    /// ## Uniform Data Object
    template<typename Struct>
    struct ubo {
        ubo(device_memory_allocator &a, Struct s)
        : allocator{a}, current{std::move(s)} {
            for (std::size_t index{}; index < max_frames_in_flight; ++index) {
                copy_to_gpu_memory(index);

                VkDescriptorBufferInfo info{};
                info.buffer = buffers[index].get();
                info.offset = 0;
                info.range = sizeof(Struct);

                VkWriteDescriptorSet set{};
                set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                set.dstSet = ubo_sets[index];
                set.dstBinding = 0;
                set.dstArrayElement = 0;
                set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                set.descriptorCount = 1;
                set.pBufferInfo = &info;

                vkUpdateDescriptorSets(a.device.get(), 1, &set, 0, nullptr);
            }
        }


        device_memory_allocator &allocator;
        Struct current;
        std::array<buffer<Struct>, max_frames_in_flight> buffers{
                buffer<Struct>{
                        allocator, 1u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
                buffer<Struct>{
                        allocator, 1u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
                buffer<Struct>{
                        allocator, 1u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}};
        std::array<device_memory::mapping, max_frames_in_flight> mappings{
                buffers[0].map(), buffers[1].map(), buffers[2].map()};
        vk::descriptor_set_layout ubo_layout{
                vk::descriptor_set_layout::for_uniform_buffer_object(
                        allocator.device)};
        vk::descriptor_pool ubo_pool{
                allocator.device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                max_frames_in_flight};
        vk::descriptor_sets ubo_sets{
                ubo_pool, ubo_layout, max_frames_in_flight};


        void copy_to_gpu_memory(std::size_t index) const {
            current.copy_to_gpu_memory(mappings[index].get());
        }
    };


}

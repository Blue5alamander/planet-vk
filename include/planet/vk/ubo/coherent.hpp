#pragma once


#include <planet/vk/buffer.hpp>
#include <planet/vk/descriptors.hpp>


namespace planet::vk::ubo {


    /// ## Memory coherent UBO wrapper
    /**
     * This is for UBOs where the data to be uploaded to the GPU can simply be
     * copied into a memory that is set to be host coherent. GPU memory is
     * allocated on construction for each frame in flight, and data is copied to
     * the GPU for each frame its needed.
     */
    template<typename Struct, std::size_t Frames>
    struct coherent {
        coherent(device_memory_allocator &a, Struct s)
        : allocator{a}, current{std::move(s)} {
            for (std::size_t index{}; index < Frames; ++index) {
                buffers[index] = {
                        allocator, 1u, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
                mappings[index] = buffers[index].map();
                copy_to_gpu_memory(index);

                VkDescriptorBufferInfo info{};
                info.buffer = buffers[index].get();
                info.offset = 0;
                info.range = sizeof(Struct);

                VkWriteDescriptorSet set{};
                set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                set.dstSet = sets[index];
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

        std::array<buffer<Struct>, Frames> buffers{};
        std::array<device_memory::mapping, Frames> mappings{};
        vk::descriptor_set_layout layout{
                vk::descriptor_set_layout::for_uniform_buffer_object(
                        allocator.device)};
        vk::descriptor_pool pool{
                allocator.device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Frames};
        vk::descriptor_sets sets{pool, layout, Frames};


        void copy_to_gpu_memory(std::size_t index) const {
            current.copy_to_gpu_memory(mappings[index].get());
        }
    };


}

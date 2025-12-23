#pragma once


#include <planet/vk/buffer.hpp>
#include <planet/vk/descriptors.hpp>

#include <cstring>


namespace planet::vk::ubo {


    /// ## Memory Coherent UBOs
    /**
     * This is for UBOs where the data to be uploaded to the GPU can simply be
     * copied into memory that is set to be host coherent. GPU memory is
     * allocated on construction for each frame in flight, and data is copied to
     * the GPU for each frame its needed.
     */

    /// ### Vulkan API details
    struct coherent_details {
        coherent_details(device &d, std::uint32_t frames)
        : layout{vk::descriptor_set_layout::for_uniform_buffer_object(d)},
          pool{d, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frames},
          sets{pool, layout, frames} {}

        vk::descriptor_set_layout layout;
        vk::descriptor_pool pool;
        vk::descriptor_sets sets;
    };


    /// ### Memory Coherent UBO
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
                set.dstSet = vk.sets[index];
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
        coherent_details vk{allocator.device, Frames};


        void copy_to_gpu_memory(std::size_t index) const {
            do_copy_to_gpu_memory(current, mappings[index].get());
        }
    };

    /**
     * The default implementation assumes the struct is suitable for `memcpy` to
     * copy it into the CPU memory that is coherently mapped to the GPU.
     *
     * Depending on the types involved, you may need to have `using
     * planet::vk::ubo::do_copy_to_gpu_memory;` in your code's namespace.
     */
    template<typename Struct>
    inline void do_copy_to_gpu_memory(Struct const &s, std::byte *d) {
        std::memcpy(d, &s, sizeof(Struct));
    }

    template<typename Struct, std::size_t M>
    inline void
            do_copy_to_gpu_memory(std::span<Struct const, M> const s, std::byte *d) {
        std::memcpy(d, s.data(), s.size_bytes());
    }
    template<typename Struct, std::size_t M>
    inline void
            do_copy_to_gpu_memory(std::array<Struct, M> const &s, std::byte *d) {
        do_copy_to_gpu_memory(std::span{s}, d);
    }


}

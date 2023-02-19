#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    /// ## Descriptor pool
    class descriptor_pool final {
        using handle_type =
                device_handle<VkDescriptorPool, vkDestroyDescriptorPool>;
        handle_type handle;

      public:
        descriptor_pool(vk::device const &, std::uint32_t count);

        vk::device const &device;
        VkDescriptorPool get() const noexcept { return handle.get(); }
    };


    /// ## Descriptor set layout
    class descriptor_set_layout final {
        using handle_type =
                device_handle<VkDescriptorSetLayout, vkDestroyDescriptorSetLayout>;
        handle_type handle;

      public:
        descriptor_set_layout(
                vk::device const &, VkDescriptorSetLayoutBinding const &);

        vk::device const &device;
        VkDescriptorSetLayout get() const noexcept { return handle.get(); }
    };


}

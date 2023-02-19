#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


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

#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;
    class swap_chain;


    /// ## Vulkan image
    class image final {
      public:
    };


    /// ## Vulkan image view
    class image_view final {
        using handle_type = device_handle<VkImageView, vkDestroyImageView>;
        handle_type handle;

      public:
        image_view(swap_chain const &, VkImage);

        vk::device const &device;
        VkImageView get() const noexcept { return handle.get(); }
    };


}

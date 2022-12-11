#pragma once


#include <planet/affine/extents2d.hpp>
#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    /// A swap chain
    class swap_chain final {
        owned_handle<VkDevice, VkSwapchainKHR, vkDestroySwapchainKHR> handle;

      public:
        swap_chain(device const &, affine::extents2d);

        static VkExtent2D extents(device const &, affine::extents2d);
    };


}

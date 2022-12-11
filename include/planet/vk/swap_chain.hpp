#pragma once


#include <planet/affine/extents2d.hpp>
#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    /// A swap chain
    class swap_chain final {
        using handle_type =
                owned_handle<VkDevice, VkSwapchainKHR, vkDestroySwapchainKHR>;
        handle_type handle;

        /// (Re-)create the swap chain and its attendant items
        std::uint32_t create(VkExtent2D);
        std::uint32_t create(affine::extents2d);

      public:
        template<typename Ex>
        swap_chain(vk::device const &d, Ex const ex) : device{d} {
            create(ex);
        }

        vk::device const &device;
        VkSwapchainKHR get() const noexcept { return handle.get(); }

        /// Recreate the swap chain and everything dependant on it, for example
        /// if the window has changed dimensions. Returns the number of images
        /// to use
        template<typename Ex>
        std::uint32_t recreate(Ex const ex) {
            device.wait_idle();
            return create(ex);
        }

        /// Calculate suitable extents to use for the swap chain given the
        /// current window size
        static VkExtent2D extents(vk::device const &, affine::extents2d);
    };


}

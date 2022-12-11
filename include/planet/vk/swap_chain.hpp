#pragma once


#include <planet/affine/extents2d.hpp>
#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;
    class image_view;


    /// A swap chain
    class swap_chain final {
        using handle_type =
                device_handle<VkSwapchainKHR, vkDestroySwapchainKHR>;
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
        static VkExtent2D
                calculate_extents(vk::device const &, affine::extents2d);
        /// The extents the current swap chain has been created for
        VkExtent2D extents;

        /// The swap chain images, and their views
        VkFormat image_format;
        std::vector<VkImage> images;
        std::vector<image_view> image_views;
    };


    /// The image views that live in the swap chain
    class image_view final {
        using handle_type = device_handle<VkImageView, vkDestroyImageView>;
        handle_type handle;

      public:
        image_view(swap_chain const &, VkImage);

        vk::device const &device;
        VkImageView get() const noexcept { return handle.get(); }
    };


}

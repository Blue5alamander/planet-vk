#pragma once


#include <planet/affine/extents2d.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/frame_buffer.hpp>
#include <planet/vk/image.hpp>
#include <planet/vk/render_pass.hpp>


namespace planet::vk {


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
        swap_chain(vk::device &d, Ex const ex) : device{d} {
            create(ex);
        }

        device_view device;
        VkSwapchainKHR get() const noexcept { return handle.get(); }

        /// Recreate the swap chain and everything dependant on it, for example
        /// if the window has changed dimensions. Returns the number of images
        /// to use
        template<typename Ex>
        std::uint32_t recreate(Ex const ex) {
            device().wait_idle();
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

        /// Frame buffers
        std::vector<frame_buffer> frame_buffers;
        template<typename... Attachments>
        void create_frame_buffers(render_pass const &rp, Attachments... extra) {
            for (auto const &view : image_views) {
                std::array attachments{VkImageView{extra}..., view.get()};
                VkFramebufferCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                info.renderPass = rp.get();
                info.attachmentCount = attachments.size();
                info.pAttachments = attachments.data();
                info.width = extents.width;
                info.height = extents.height;
                info.layers = 1;
                frame_buffers.emplace_back(device, info);
            }
        }
    };


}

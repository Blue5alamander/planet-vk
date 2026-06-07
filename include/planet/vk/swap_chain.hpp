#pragma once


#include <planet/affine/extents2d.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/frame_buffer.hpp>
#include <planet/vk/image.hpp>
#include <planet/vk/render_pass.hpp>


namespace planet::vk {


    /// ## Transfer source usage for the swap chain images
    /**
     * Whether the swap chain images were created with
     * `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` so their contents can be copied out
     * (for example to save a screenshot). Pass `requested` at construction to
     * ask for it; after creation the value resolves to `available` or
     * `not_available` depending on what the surface supports.
     */
    enum class transfer_source {
        /// Transfer-source usage was not asked for
        not_requested,
        /// Transfer-source usage was asked for (not yet resolved)
        requested,
        /// Requested and supported -- the images carry the usage bit
        available,
        /// Requested but the surface does not support it
        not_available,
    };


    /// ## A swap chain
    class swap_chain final {
        using handle_type =
                device_handle<VkSwapchainKHR, vkDestroySwapchainKHR>;
        handle_type handle;

        /// ### (Re-)create the swap chain and its attendant items
        std::uint32_t create(VkExtent2D);
        std::uint32_t create(affine::extents2d);


      public:
        /// ### Creation
        /**
         * Pass `transfer_source::requested` to ask for the images to be created
         * with `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` so their contents can be
         * copied out (for example to save a screenshot). The request is only
         * honoured when the surface supports it -- check `transfer` to see
         * whether it actually happened.
         */
        template<typename Ex>
        swap_chain(
                vk::device &d,
                Ex const ex,
                vk::transfer_source const request =
                        transfer_source::not_requested)
        : device{d}, transfer{request} {
            create(ex);
        }


        /// ### Queries
        device_view device;

        VkSwapchainKHR get() const noexcept { return handle.get(); }


        /// ### Recreation
        /**
         * Recreate the swap chain and everything dependant on it, for example
         * if the window has changed dimensions. Returns the number of images to
         * use
         */
        template<typename Ex>
        std::uint32_t recreate(Ex const ex) {
            device().wait_idle();
            return create(ex);
        }


        /// ### Extents

        /// #### Calculation
        /**
         * Calculate suitable extents to use for the swap chain given the
         * current window size
         */
        static VkExtent2D
                calculate_extents(vk::device const &, affine::extents2d);
        /// #### The extents the current swap chain has been created for
        VkExtent2D extents;


        /// ### Whether the images can be used as a transfer source
        /**
         * Resolves to `transfer_source::available` when transfer-source usage
         * was requested at construction *and* the surface supports it, meaning
         * the images were created with `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` and
         * their contents can be copied out.
         */
        transfer_source transfer;


        /// ### The swap chain images, and their views
        VkFormat image_format;
        std::vector<VkImage> images;
        std::vector<image_view> image_views;


        /// ### Frame buffers
        std::vector<frame_buffer> frame_buffers;
        template<typename... Attachments>
        void create_frame_buffers(render_pass const &rp, Attachments... extra) {
            frame_buffers.clear();
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


        /// ### Resolve attachment
        VkAttachmentDescription attachment_description() const;
    };


}

#pragma once


#include <planet/vk/image.hpp>


namespace planet::vk::engine {


    /// ## Colour attachment
    struct colour_attachment {
        /// ### Construction
        colour_attachment(
                device_memory_allocator &,
                swap_chain &,
                VkImageUsageFlagBits = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
        /**
         * The attachment will always get `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`.
         * Depending on usage either `VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT`
         * or `VK_IMAGE_USAGE_SAMPLED_BIT` also needs to be passed in.
         */


        vk::image image;
        vk::image_view image_view;


        VkAttachmentDescription
                attachment_description(physical_device const &) const;
        VkAttachmentDescription attachment_description(
                VkSampleCountFlagBits, VkAttachmentLoadOp) const;
    };


}

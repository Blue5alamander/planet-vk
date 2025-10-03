#pragma once


#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/image.hpp>


namespace planet::vk::engine {


    /// ## Colour attachment
    struct colour_attachment {
        /// ### Construction
        struct parameters {
            /// #### GPU memory allocator
            device_memory_allocator &allocator;

            /// #### Extents and format
            VkExtent2D extents;
            VkFormat format;

            /// #### Flags
            VkImageUsageFlagBits usage_flags =
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            /**
             * The attachment will always get
             * `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`. Depending on usage either
             * `VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT` or
             * `VK_IMAGE_USAGE_SAMPLED_BIT` also needs to be passed in.
             */

            VkSampleCountFlagBits sample_count =
                    usage_flags == VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                    ? allocator.device->instance.gpu().msaa_samples
                    : VK_SAMPLE_COUNT_1_BIT;
            /**
             * The default uses the MSAA mode from the GPU to determine the
             * correct sample count, but in the case of an attachment used for
             * the final colour, which is being sampled for the next stage in
             * the render pass, the sample count should be 1.
             */
        };
        colour_attachment(parameters);


        vk::image image;
        vk::image_view image_view;


        VkAttachmentDescription
                attachment_description(physical_device const &) const;
        VkAttachmentDescription attachment_description(
                VkSampleCountFlagBits, VkAttachmentLoadOp) const;
    };


}

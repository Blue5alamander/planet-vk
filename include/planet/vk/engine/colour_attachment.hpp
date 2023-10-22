#pragma once


#include <planet/vk/image.hpp>


namespace planet::vk::engine {


    struct colour_attachment {
        colour_attachment(swap_chain &);

        vk::image image;
        vk::image_view image_view;

        VkAttachmentDescription
                attachment_description(physical_device const &) const;
    };


}

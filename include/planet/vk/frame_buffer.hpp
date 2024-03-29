#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class frame_buffer final {
        using handle_type = device_handle<VkFramebuffer, vkDestroyFramebuffer>;
        handle_type handle;

      public:
        frame_buffer(vk::device &, VkFramebufferCreateInfo const &);

        device_view device;
        VkFramebuffer get() const noexcept { return handle.get(); }
    };


}

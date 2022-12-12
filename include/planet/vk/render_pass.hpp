#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class render_pass final {
        using handle_type = device_handle<VkRenderPass, vkDestroyRenderPass>;
        handle_type handle;

      public:
        render_pass(vk::device const &, VkRenderPassCreateInfo const &);

        vk::device const &device;
        VkRenderPass get() const noexcept { return handle.get(); }
    };


}

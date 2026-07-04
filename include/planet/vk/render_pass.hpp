#pragma once


#include <planet/vk/helpers.hpp>
#include <planet/vk/owned_handle.hpp>
#include <planet/vk/view.hpp>


namespace planet::vk {


    class render_pass final {
        using handle_type = device_handle<VkRenderPass, vkDestroyRenderPass>;
        handle_type handle;

      public:
        render_pass(vk::device &, VkRenderPassCreateInfo const &);

        device_view device;
        VkRenderPass get() const noexcept { return handle.get(); }
    };


}

#pragma once


#include <planet/vk/helpers.hpp>
#include <planet/vk/owned_handle.hpp>
#include <planet/vk/view.hpp>


namespace planet::vk {


    class device;


    class fence final {
        using handle_type = device_handle<VkFence, vkDestroyFence>;
        handle_type handle;

      public:
        fence(vk::device &);

        device_view device;
        VkFence get() const noexcept { return handle.get(); }

        /// ### Returns true if the fence is ready
        bool is_ready() const;

        /// ### Resets the fence
        void reset();
    };


    class semaphore final {
        using handle_type = device_handle<VkSemaphore, vkDestroySemaphore>;
        handle_type handle;

      public:
        semaphore(vk::device &);

        device_view device;
        VkSemaphore get() const noexcept { return handle.get(); }
    };


}

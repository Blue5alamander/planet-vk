#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    class fence final {
        using handle_type = device_handle<VkFence, vkDestroyFence>;
        handle_type handle;

      public:
        fence(vk::device const &);

        vk::device const &device;
        VkFence get() const noexcept { return handle.get(); }
    };


    class semaphore final {
        using handle_type = device_handle<VkSemaphore, vkDestroySemaphore>;
        handle_type handle;

      public:
        semaphore(vk::device const &);

        vk::device const &device;
        VkSemaphore get() const noexcept { return handle.get(); }
    };


}

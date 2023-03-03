#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    struct extensions;
    class instance;


    /// The logical graphics device we're using
    class device final {
        VkDevice handle = VK_NULL_HANDLE;

      public:
        device(vk::instance const &, extensions const &);
        ~device();

        vk::instance const &instance;

        device(device const &) = delete;
        device(device &&) = delete;
        device &operator=(device const &) = delete;
        device &operator=(device &&i) = delete;

        VkDevice get() const noexcept { return handle; }

        VkQueue graphics_queue = VK_NULL_HANDLE, present_queue = VK_NULL_HANDLE;

        /// Wait for the device to become idle
        void wait_idle() const { vkDeviceWaitIdle(handle); }
    };


}

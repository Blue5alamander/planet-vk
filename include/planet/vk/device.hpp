#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    struct extensions;
    class instance;


    /// The logical graphics device we're using
    class device final {
        VkDevice handle = VK_NULL_HANDLE;

      public:
        device() noexcept;
        explicit device(instance const &, extensions const &);
        ~device();

        VkDevice get() const noexcept { return handle; }

        VkQueue graphics_queue = VK_NULL_HANDLE, present_queue = VK_NULL_HANDLE;
    };


}

#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    struct extensions;


    /// Return a filled in structure that can be used to create the instance
    VkApplicationInfo application_info();


    /// The Vulkan instance
    class instance final {
        VkInstance handle = VK_NULL_HANDLE;
        void reset() noexcept;

      public:
        static VkInstanceCreateInfo
                info(extensions const &, VkApplicationInfo const &);
        instance() noexcept {}
        explicit instance(VkInstanceCreateInfo const &);
        ~instance() { reset(); }

        instance &operator=(instance &&i) noexcept {
            reset();
            handle = std::exchange(i.handle, VK_NULL_HANDLE);
            return *this;
        }

        VkInstance get() noexcept { return handle; }
    };


}

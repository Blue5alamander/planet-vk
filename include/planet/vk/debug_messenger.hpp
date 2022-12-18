#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class instance;


    /// Debug messenger instance
    class debug_messenger final {
        VkInstance instance_handle = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT handle = VK_NULL_HANDLE;

      public:
        debug_messenger() {}
        debug_messenger(debug_messenger const &) = delete;
        debug_messenger(debug_messenger &&) = delete;
        debug_messenger(
                vk::instance const &,
                PFN_vkDebugUtilsMessengerCallbackEXT,
                void * = nullptr);
        ~debug_messenger();

        /// Fill in the create info structure
        static VkDebugUtilsMessengerCreateInfoEXT create_info(
                PFN_vkDebugUtilsMessengerCallbackEXT, void * = nullptr);

        debug_messenger &operator=(debug_messenger const &) = delete;
        debug_messenger &operator=(debug_messenger &&o) {
            std::swap(instance_handle, o.instance_handle);
            std::swap(handle, o.handle);
            return *this;
        }

        VkDebugUtilsMessengerEXT get() const noexcept { return handle; }
    };


}

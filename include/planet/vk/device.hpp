#pragma once


#include <planet/vk/memory.hpp>


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

        /// #### Wait for the device to become idle
        void wait_idle() const { vkDeviceWaitIdle(handle); }


        /// ### Allocators

        /// #### Allocate start-up memory
        /**
         * Use this to allocate device memory that is used for very long life
         * times. For example, geometry or textures that art loaded at start up
         * and then used for the entire life of the application.
         */
        device_memory_allocator startup_memory{"startup", *this};

        /// #### Allocate staging memory
        /**
         * Use this for staging memory. The buffers or images etc. allocated
         * here should be for very short lived allocations.
         */
        device_memory_allocator staging_memory{"staging", *this};
    };


}

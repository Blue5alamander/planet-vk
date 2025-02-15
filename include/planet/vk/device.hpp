#pragma once


#include <planet/vk/memory.hpp>
#include <planet/vk/queue.hpp>

#include <mutex>


namespace planet::vk {


    struct extensions;
    class instance;


    /// ## Vulkan device
    /// The logical graphics device we're using
    class device final {
        friend class queue;


        VkDevice handle = VK_NULL_HANDLE;

        std::mutex transfer_queue_mutex;
        std::optional<std::pair<VkQueue, std::uint32_t>> held_transfer_queue =
                {};
        void return_transfer_queue(VkQueue, std::uint32_t);


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


        /// ### Fetch a transfer queue
        /// If there is no transfer queue left then it will return an empty
        /// `vk::queue`
        vk::queue transfer_queue();


        /// ### Wait for the device to become idle
        void wait_idle() const;


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

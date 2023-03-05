#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class command_pool;
    class device;
    class surface;


    class command_buffer final {
        friend class command_buffers;
        /// These instances are owned by the command_buffers type
        VkCommandBuffer handle;

        /**
         * The command buffer can be either a one-off used for a particular
         * purpose, or it can be owned by the `command_buffers` structure. This
         * flags tells the destructor which is which.
         */
        bool self_owned = false;
        /// Used when the `command_buffers` instance owns the handles
        command_buffer(vk::command_pool const &, VkCommandBuffer);

      public:
        explicit command_buffer(
                vk::command_pool const &,
                VkCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        command_buffer(command_buffer const &) = delete;
        command_buffer(command_buffer &&);
        ~command_buffer();

        command_buffer &operator=(command_buffer const &) = delete;
        command_buffer &operator=(command_buffer &&) = delete;

        device_view device;
        vk::command_pool const &command_pool;
        auto get() const noexcept { return handle; }


        /// ### API wrappers

        /// #### `vkBeginCommandBuffer`
        void begin(VkCommandBufferUsageFlags = {});
        /// #### `vkEndCommandBuffer`
        void end();
        /// #### `vkQueueSubmit`
        void submit(VkQueue);
    };


    class command_buffers final {
        std::vector<VkCommandBuffer> handles;
        std::vector<command_buffer> buffers;

      public:
        command_buffers(
                vk::command_pool const &,
                std::size_t number_of_buffers,
                VkCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        command_buffers(command_buffers const &) = delete;
        command_buffers(command_buffers &&) = delete;
        ~command_buffers();

        command_buffers &operator=(command_buffers const &) = delete;
        command_buffers &operator=(command_buffers &&) = delete;

        device_view device;
        vk::command_pool const &command_pool;

        std::size_t size() const noexcept { return buffers.size(); }
        command_buffer &operator[](std::size_t const i) {
            return buffers.at(i);
        }
    };


    class command_pool final {
        using handle_type = device_handle<VkCommandPool, vkDestroyCommandPool>;
        handle_type handle;

      public:
        command_pool(vk::device &, vk::surface const &);

        device_view device;
        VkCommandPool get() const noexcept { return handle.get(); }
    };


}

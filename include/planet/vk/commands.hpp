#pragma once


#include <planet/vk/forward.hpp>
#include <planet/vk/owned_handle.hpp>
#include <planet/vk/queue.hpp>
#include <planet/vk/view.hpp>

#include <vector>


namespace planet::vk {


    /// ## Command buffer
    class command_buffer final {
        friend class command_buffers;
        /// These instances are owned by the command_buffers type
        VkCommandBuffer handle;
        VkQueue queue;

        /**
         * The command buffer can be either a one-off used for a particular
         * purpose, or it can be owned by the `command_buffers` structure. This
         * flags tells the destructor which is which.
         */
        bool self_owned = false;
        /// Used when the `command_buffers` instance owns the handles
        command_buffer(vk::command_pool &, VkCommandBuffer);


      public:
        command_buffer() {}
        explicit command_buffer(
                vk::command_pool &,
                VkCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        command_buffer(command_buffer const &) = delete;
        command_buffer(command_buffer &&);
        ~command_buffer() { reset(); }

        command_buffer &operator=(command_buffer const &) = delete;
        command_buffer &operator=(command_buffer &&);

        device_view device;
        view<vk::command_pool> command_pool;
        auto get() const noexcept { return handle; }


        /// ### Single use graphics command buffer

        /// #### Start a single use command buffer
        static command_buffer single_use(vk::command_pool &);
        /// #### End and submit on the graphics queue
        /**
         * As well as combining the `end` with a `submit` to the graphics queue,
         * it also waits for the queue to become idle again.
         */
        void end_and_submit();


        /// ### API wrappers

        /// #### `vkBeginCommandBuffer`
        void begin(VkCommandBufferUsageFlags = {});
        /// #### `vkEndCommandBuffer`
        void end();
        /// #### `vkQueueSubmit`
        /**
         * The queue that is used is the one given to the original
         * `command_pool`, or the graphics queue if the `command_pool` is given
         * a surface.
         */
        void submit();


      private:
        void reset();
    };


    /// ## Command buffers
    class command_buffers final {
        std::vector<VkCommandBuffer> handles;
        std::vector<command_buffer> buffers;


      public:
        command_buffers(
                vk::command_pool &,
                std::size_t number_of_buffers,
                VkCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        command_buffers(command_buffers const &) = delete;
        command_buffers(command_buffers &&) = delete;
        ~command_buffers();

        command_buffers &operator=(command_buffers const &) = delete;
        command_buffers &operator=(command_buffers &&) = delete;

        device_view device;
        view<vk::command_pool> command_pool;

        std::size_t size() const noexcept { return buffers.size(); }
        command_buffer &operator[](std::size_t const i) {
            return buffers.at(i);
        }
    };


    /// ## Command pool
    class command_pool final {
        using handle_type = device_handle<VkCommandPool, vkDestroyCommandPool>;
        handle_type handle;
        vk::queue queue;


      public:
        command_pool(vk::device &, vk::surface const &);
        command_pool(vk::device &, vk::queue);


        device_view device;
        VkCommandPool get() const noexcept { return handle.get(); }
        VkQueue command_queue();
    };


}

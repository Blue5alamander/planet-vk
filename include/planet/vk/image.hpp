#pragma once


#include <planet/vk/memory.hpp>


namespace planet::vk {


    class device;
    class swap_chain;


    /// ## Vulkan image
    /**
     * Unlike a regular `VkImage`, this type also owns the device memory needed
     * to back the image.
     */
    class image final {
        using handle_type = vk::device_handle<VkImage, vkDestroyImage>;
        handle_type handle;

        device_memory memory;

      public:
        /// ### Image creation
        image() {}
        image(device_memory_allocator &,
              std::uint32_t width,
              std::uint32_t height,
              std::uint32_t mip_levels,
              VkSampleCountFlagBits,
              VkFormat,
              VkImageTiling,
              VkImageUsageFlags,
              VkMemoryPropertyFlags);


        /// ### Query image view

        auto device_handle() const noexcept { return handle.owner(); }
        auto get() const noexcept { return handle.get(); }
    };


    /// ## Vulkan image view
    class image_view final {
        using handle_type = vk::device_handle<VkImageView, vkDestroyImageView>;
        handle_type handle;

      public:
        /// ### Image view creation

        image_view() {}

        /// #### Create from an image
        image_view(
                VkDevice,
                VkImage,
                VkFormat,
                VkImageAspectFlags,
                std::uint32_t mip_levels);
        image_view(
                vk::image const &image,
                VkFormat const format,
                VkImageAspectFlags const flags,
                std::uint32_t const mip_levels)
        : image_view{
                image.device_handle(), image.get(), format, flags,
                mip_levels} {}

        /// #### Create an image view from the swap chain
        image_view(swap_chain const &, VkImage);


        /// ### Query image view

        auto device_handle() const noexcept { return handle.owner(); }
        auto get() const noexcept { return handle.get(); }
    };


}
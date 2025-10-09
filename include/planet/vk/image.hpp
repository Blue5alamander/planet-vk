#pragma once


#include <planet/affine/extents2d.hpp>
#include <planet/vk/forward.hpp>
#include <planet/vk/memory.hpp>


namespace planet::vk {


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

        std::uint32_t width = {}, height = {}, mip_levels = {};
        VkFormat format = {};
        auto device_handle() const noexcept { return handle.owner(); }
        auto get() const noexcept { return handle.get(); }

        affine::extents2d extents() const noexcept {
            return {static_cast<float>(width), static_cast<float>(height)};
        }

        /// ### Image manipulation

        /// #### Set up a layout transition
        void transition_layout(
                command_pool &,
                VkImageLayout old_layout,
                VkImageLayout new_layout,
                std::uint32_t mip_levels);

        /// #### Copy staging buffer pixels to the image
        void copy_from(command_pool &, buffer<std::byte> const &);

        /// #### Generate MIP map layers for image
        void generate_mip_maps(command_pool &, std::uint32_t mip_levels);
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
        image_view(vk::image const &image, VkImageAspectFlags const flags)
        : image_view{
                  image.device_handle(), image.get(), image.format, flags,
                  image.mip_levels} {}

        /// #### Create an image view from the swap chain
        image_view(swap_chain const &, VkImage);


        /// ### Query image view

        auto device_handle() const noexcept { return handle.owner(); }
        auto get() const noexcept { return handle.get(); }
    };


}

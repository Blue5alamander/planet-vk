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


        /// ### Attributes
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        std::uint32_t width = {}, height = {}, mip_levels = {};
        VkFormat format = {};


        /// ### Query image view
        auto device_handle() const noexcept { return handle.owner(); }
        auto get() const noexcept { return handle.get(); }

        affine::extents2d extents() const noexcept {
            return {static_cast<float>(width), static_cast<float>(height)};
        }


        /// ### Image manipulation

        /// #### Transition memory barrier
        struct transition_parameters {
            std::optional<VkImageLayout> old_layout = {};
            VkImageLayout new_layout;
            VkAccessFlags source_access_mask = VK_ACCESS_NONE,
                          destination_access_mask;
            std::uint32_t source_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
                          destination_queue_family_index =
                                  VK_QUEUE_FAMILY_IGNORED;
            std::uint32_t base_mip_level = {}, mip_levels = 1;
        };
        VkImageMemoryBarrier transition(transition_parameters const p) {
            return VkImageMemoryBarrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = p.source_access_mask,
                    .dstAccessMask = p.destination_access_mask,
                    .oldLayout = p.old_layout
                            ? *p.old_layout
                            : std::exchange(layout, p.new_layout),
                    .newLayout = p.new_layout,
                    .srcQueueFamilyIndex = p.source_queue_family_index,
                    .dstQueueFamilyIndex = p.destination_queue_family_index,
                    .image = get(),
                    .subresourceRange = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = p.base_mip_level,
                            .levelCount = p.mip_levels,
                            .baseArrayLayer = {},
                            .layerCount = 1}};
        }
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

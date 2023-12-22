#pragma once


#include <planet/affine/rectangle2d.hpp>
#include <planet/vk/image.hpp>
#include <planet/ui/scale.hpp>


namespace planet::vk {


    class device;


    /// ## Sampler
    class sampler final {
        using handle_type = vk::device_handle<VkSampler, vkDestroySampler>;
        handle_type handle;

      public:
        /// ### Construct
        sampler() {}
        sampler(vk::device &, std::uint32_t mip_levels);


        /// ### Query sampler

        device_view device;
        auto get() const noexcept { return handle.get(); }
    };


    /// ## Texture map
    class texture final {
      public:
        /// ### Construction
        struct args {
            device_memory_allocator &allocator;
            vk::command_pool &command_pool;
            vk::buffer<std::byte> const &buffer;
            std::uint32_t width;
            std::uint32_t height;
            ui::scale scale = ui::scale::lock_aspect;
            VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
        };
        static texture create_with_mip_levels_from(args);
        static texture create_without_mip_levels_from(args);


        /// ### Attributes
        vk::image image;
        vk::image_view image_view;
        vk::sampler sampler;
        ui::scale fit = ui::scale::lock_aspect;

        /// ### Queries
        explicit operator bool() const noexcept;
    };


    /// ## A rectangular area within another texture
    /// Used for sprite sheets
    using sub_texture =
            std::pair<vk::texture const &, planet::affine::rectangle2d>;


}

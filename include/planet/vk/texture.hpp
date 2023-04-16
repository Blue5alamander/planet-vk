#pragma once


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
        static texture create_with_mip_levels_from(
                device_memory_allocator &,
                command_pool &,
                buffer<std::byte> const &,
                std::uint32_t width,
                std::uint32_t height,
                ui::scale = ui::scale::lock_aspect);


        /// ### Attributes
        vk::image image;
        vk::image_view image_view;
        vk::sampler sampler;
        ui::scale fit = ui::scale::lock_aspect;

        /// ### Queries
        explicit operator bool() const noexcept;
    };


}

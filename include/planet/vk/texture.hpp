#pragma once


#include <planet/vk/image.hpp>


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
        std::uint32_t mip_levels = {};
        vk::image image;
        vk::image_view image_view;
        vk::sampler sampler;
    };


}

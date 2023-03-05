#pragma once


#include <planet/vk/helpers.hpp>


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

        auto device_handle() const noexcept { return handle.owner(); }
        auto get() const noexcept { return handle.get(); }
    };


    /// ## Texture map
    class texture final {
      public:
    };


}

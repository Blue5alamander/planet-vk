#pragma once


#include <planet/vk/helpers.hpp>


namespace planet::vk {


    class device;


    class shader_module final {
        using handle_type =
                device_handle<VkShaderModule, vkDestroyShaderModule>;
        handle_type handle;

        std::vector<std::byte> spirv;

      public:
        shader_module(vk::device const &, std::vector<std::byte>);

        vk::device const &device;
        VkShaderModule get() const noexcept { return handle.get(); }
    };


}

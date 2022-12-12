#pragma once


#include <planet/vk/render_pass.hpp>


namespace planet::vk {


    class pipeline_layout final {
        using handle_type =
                device_handle<VkPipelineLayout, vkDestroyPipelineLayout>;
        handle_type handle;

      public:
        pipeline_layout(vk::device const &);
        pipeline_layout(vk::device const &, VkPipelineLayoutCreateInfo const &);

        vk::device const &device;
        VkPipelineLayout get() const noexcept { return handle.get(); }
    };


    class graphics_pipeline final {
        using handle_type = device_handle<VkPipeline, vkDestroyPipeline>;

      public:
        graphics_pipeline(
                vk::device const &,
                VkGraphicsPipelineCreateInfo &,
                vk::render_pass,
                pipeline_layout);

        vk::device const &device;
        VkPipeline get() const noexcept { return handle.get(); }

        vk::render_pass render_pass;
        vk::pipeline_layout layout;

      private:
        handle_type handle;
    };


}

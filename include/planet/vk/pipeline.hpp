#pragma once


#include <planet/vk/render_pass.hpp>


namespace planet::vk {


    class descriptor_set_layout;


    class pipeline_layout final {
        using handle_type =
                device_handle<VkPipelineLayout, vkDestroyPipelineLayout>;
        handle_type handle;

      public:
        explicit pipeline_layout(vk::device &);
        explicit pipeline_layout(vk::device &, descriptor_set_layout const &);
        explicit pipeline_layout(
                vk::device &,
                std::span<VkDescriptorSetLayout const>,
                std::span<VkPushConstantRange const> = {});
        explicit pipeline_layout(
                vk::device &, VkPipelineLayoutCreateInfo const &);

        device_view device;
        auto get() const noexcept { return handle.get(); }
    };


    class graphics_pipeline final {
        using handle_type = device_handle<VkPipeline, vkDestroyPipeline>;

      public:
        graphics_pipeline(
                vk::device &,
                VkGraphicsPipelineCreateInfo &,
                vk::render_pass &,
                pipeline_layout);

        device_view device;
        auto get() const noexcept { return handle.get(); }

        view<vk::render_pass> render_pass;
        vk::pipeline_layout layout;

      private:
        handle_type handle;
    };


}

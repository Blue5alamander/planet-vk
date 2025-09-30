#pragma once


#include <planet/vk/render_pass.hpp>

#include <span>


namespace planet::vk {


    class descriptor_set_layout;


    class pipeline_layout final {
        using handle_type =
                device_handle<VkPipelineLayout, vkDestroyPipelineLayout>;
        handle_type handle;

      public:
        pipeline_layout() {}
        pipeline_layout(pipeline_layout const &) = delete;
        pipeline_layout(pipeline_layout &&) = default;
        explicit pipeline_layout(vk::device &);
        explicit pipeline_layout(vk::device &, descriptor_set_layout const &);
        explicit pipeline_layout(
                vk::device &,
                std::span<VkDescriptorSetLayout const>,
                std::span<VkPushConstantRange const> = {});
        explicit pipeline_layout(
                vk::device &, VkPipelineLayoutCreateInfo const &);

        pipeline_layout &operator=(pipeline_layout const &) = delete;
        pipeline_layout &operator=(pipeline_layout &&) = default;


        device_view device;
        auto get() const noexcept { return handle.get(); }
    };


    class graphics_pipeline final {
        using handle_type = device_handle<VkPipeline, vkDestroyPipeline>;

      public:
        graphics_pipeline() {}
        graphics_pipeline(graphics_pipeline const &) = delete;
        graphics_pipeline(graphics_pipeline &&) = default;
        graphics_pipeline(
                vk::device &,
                VkGraphicsPipelineCreateInfo &,
                vk::render_pass &,
                pipeline_layout);

        graphics_pipeline &operator=(graphics_pipeline const &) = delete;
        graphics_pipeline &operator=(graphics_pipeline &&) = default;


        device_view device;
        auto get() const noexcept { return handle.get(); }

        view<vk::render_pass> render_pass;
        vk::pipeline_layout layout;

      private:
        handle_type handle;
    };


}

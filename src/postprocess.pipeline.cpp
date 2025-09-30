#include <planet/vk/engine/pipeline/postprocess.hpp>
#include <planet/vk/engine/renderer.hpp>


using namespace std::literals;


/// ## `planet::vk::engine::pipeline::postprocess`


planet::vk::engine::pipeline::postprocess::postprocess(parameters p)
: sampler_layout{p.renderer.app.device, VkDescriptorSetLayoutBinding{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .pImmutableSamplers = nullptr}},
  sampler{
          {.device = p.renderer.app.device,
           .address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE}},
  descriptor_pool{
          p.renderer.app.device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<std::uint32_t>(max_frames_in_flight)},
  descriptor_sets{descriptor_pool, sampler_layout, max_frames_in_flight},
  pipeline{create_graphics_pipeline(
          {.app = p.renderer.app,
           .renderer = p.renderer,
           .vertex_shader = {"planet-vk-engine/postprocess.vert.spirv"sv},
           .fragment_shader = {"planet-vk-engine/postprocess.copy.frag.spirv"sv},
           .binding_descriptions = {},
           .attribute_descriptions = {},
           .render_pass = p.renderer.present_render_pass,
           .write_to_depth_buffer = false,
           .multisampling = VK_SAMPLE_COUNT_1_BIT,
           .blend_mode = blend_mode::none,
           .pipeline_layout =
                   pipeline_layout{p.renderer.app.device, sampler_layout}})} {}

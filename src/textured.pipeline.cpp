#include <planet/vk/engine2d/renderer.hpp>
#include <planet/vk/engine2d/textured.pipeline.hpp>


/// ## `planet::vk::engine2d::pipeline::textured`


namespace {


    constexpr auto binding_description{[]() {
        VkVertexInputBindingDescription description{};

        description.binding = 0;
        description.stride =
                sizeof(planet::vk::engine2d::pipeline::textured::vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::array{description};
    }()};
    constexpr auto attribute_description{[]() {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[0].offset =
                offsetof(planet::vk::engine2d::pipeline::textured::vertex, p);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[1].offset =
                offsetof(planet::vk::engine2d::pipeline::textured::vertex, uv);

        return attrs;
    }()};


}


planet::vk::engine2d::pipeline::textured::textured(
        engine2d::app &a,
        vk::swap_chain &sc,
        vk::render_pass &rp,
        vk::descriptor_set_layout &dsl)
: app{a},
  swap_chain{sc},
  render_pass{rp},
  vp_layout{dsl},
  texture_layout{[&]() {
      VkDescriptorSetLayoutBinding binding{};
      binding.binding = 0;
      binding.descriptorCount = 1;
      binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      binding.pImmutableSamplers = nullptr;
      binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      return vk::descriptor_set_layout{app.device, binding};
  }()},
  texture_sets{
          vk::descriptor_sets{
                  texture_pool, texture_layout, max_textures_per_frame},
          vk::descriptor_sets{
                  texture_pool, texture_layout, max_textures_per_frame},
          vk::descriptor_sets{
                  texture_pool, texture_layout, max_textures_per_frame}} {}


planet::vk::graphics_pipeline
        planet::vk::engine2d::pipeline::textured::create_pipeline() {
    return planet::vk::engine2d::create_graphics_pipeline(
            app, "planet-vk-engine2d/textured.vert.spirv",
            "planet-vk-engine2d/textured.frag.spirv", binding_description,
            attribute_description, swap_chain, render_pass,
            {app.device, std::array{vp_layout.get(), texture_layout.get()}});
}


void planet::vk::engine2d::pipeline::textured::draw(
        vk::texture const &texture, affine::rectangle2d const pos) {
    if (textures.size() == max_textures_per_frame) {
        throw felspar::stdexcept::runtime_error{
                "Have run out of texture slots for this frame"};
    }

    std::size_t const quad_index = quads.size();

    quads.push_back({{pos.bottom_right().x(), pos.bottom_right().y()}, {1, 1}});
    quads.push_back({{pos.bottom_right().x(), pos.top_left.y()}, {1, 0}});
    quads.push_back({{pos.top_left.x(), pos.top_left.y()}, {0, 0}});
    quads.push_back({{pos.top_left.x(), pos.bottom_right().y()}, {0, 1}});

    indexes.push_back(quad_index);
    indexes.push_back(quad_index + 2);
    indexes.push_back(quad_index + 1);
    indexes.push_back(quad_index);
    indexes.push_back(quad_index + 3);
    indexes.push_back(quad_index + 2);

    textures.emplace_back();
    textures.back().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textures.back().imageView = texture.image_view.get();
    textures.back().sampler = texture.sampler.get();
}


void planet::vk::engine2d::pipeline::textured::render(
        engine2d::renderer &renderer,
        command_buffer &cb,
        std::size_t const current_frame) {
    if (quads.empty()) { return; }

    auto &vertex_buffer = vertex_buffers[current_frame];
    vertex_buffer = {
            renderer.per_frame_memory, quads, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    auto &index_buffer = index_buffers[current_frame];
    index_buffer = {
            renderer.per_frame_memory, indexes,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    std::array buffers{vertex_buffer.get()};
    std::array offset{VkDeviceSize{}};

    vkCmdBindVertexBuffers(
            cb.get(), 0, buffers.size(), buffers.data(), offset.data());
    vkCmdBindIndexBuffer(cb.get(), index_buffer.get(), 0, VK_INDEX_TYPE_UINT32);

    for (std::size_t index{}; auto const &tx : textures) {
        VkWriteDescriptorSet wds{};
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        wds.dstSet = texture_sets[current_frame][index];
        wds.dstBinding = 0;
        wds.dstArrayElement = 0;
        wds.descriptorCount = 1;
        wds.pImageInfo = &tx;
        vkUpdateDescriptorSets(app.device.get(), 1, &wds, 0, nullptr);

        vkCmdBindDescriptorSets(
                cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout.get(), 1, 1,
                &texture_sets[current_frame][index], 0, nullptr);

        constexpr std::uint32_t index_count = 6;
        constexpr std::uint32_t instance_count = 1;
        constexpr std::int32_t vertex_offset = 0;
        constexpr std::uint32_t first_instance = 0;
        vkCmdDrawIndexed(
                cb.get(), index_count, instance_count, index * index_count,
                vertex_offset, first_instance);

        ++index;
    }

    // Clear out data from this frame
    quads.clear();
    indexes.clear();
    textures.clear();
}

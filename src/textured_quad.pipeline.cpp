#include <planet/functional.hpp>
#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/engine/pipeline/textured_quad.hpp>


/// ## `planet::vk::engine::pipeline::textured_quad`


planet::vk::engine::pipeline::textured_quad::textured_quad(parameters const p)
: id{p.name, p.use_name_suffix},
  textures_ubo{
          std::string{name()} + "__textures_ubo", p.renderer.app.device,
          p.textures_per_frame},
  pipeline{planet::vk::engine::create_graphics_pipeline(
          {.app = p.renderer.app,
           .renderer = p.renderer,
           .vertex_shader = p.vertex_shader,
           .fragment_shader = p.fragment_shader,
           .binding_descriptions = vertex::binding_description<vertex_type>(),
           .attribute_descriptions =
                   vertex::attribute_description<vertex_type>(),
           .pipeline_layout = pipeline_layout{
                   p.renderer.app.device,
                   std::array{
                           p.renderer.coordinates_ubo_layout().get(),
                           textures_ubo.layout.get()}}})} {}


void planet::vk::engine::pipeline::textured_quad::draw(
        vk::sub_texture const &texture,
        affine::rectangle2d const &pos,
        planet::colour const &colour,
        float const z) {
    commands.push_back(
            &texture.first,
            quad_draw_info{
                    .position = pos,
                    .uv = texture.second,
                    .colour = colour,
                    .z = z});
}


/// ### Rendering
void planet::vk::engine::pipeline::textured_quad::render(render_parameters rp) {
    auto const texture_count = commands.non_empty_count();
    if (texture_count == 0) { return; }

    textures_ubo.textures_in_frame.value(texture_count);
    if (texture_count > textures_ubo.max_per_frame) {
        planet::log::critical(
                "We will run out of texture slots for this frame");
    }

    /// #### Pass 1
    /// Build combined vertex and index buffers across all textures
    vertices.clear();
    indices.clear();
    for (auto const &[texture, cmds] : commands.non_empty_vectors()) {
        for (auto const &cmd : cmds) {
            auto const quad_index = static_cast<std::uint32_t>(vertices.size());

            auto const &pos = cmd.position;
            auto const &uv = cmd.uv;
            auto const uv_br = uv.bottom_right();

            vertices.push_back(
                    {{pos.top_left.xh + pos.extents.width,
                      pos.top_left.yh + pos.extents.height, cmd.z},
                     cmd.colour,
                     {uv_br.xh, uv_br.yh}});
            vertices.push_back(
                    {{pos.top_left.xh + pos.extents.width, pos.top_left.yh,
                      cmd.z},
                     cmd.colour,
                     {uv_br.xh, uv.top_left.yh}});
            vertices.push_back(
                    {{pos.top_left.xh, pos.top_left.yh, cmd.z},
                     cmd.colour,
                     {uv.top_left.xh, uv.top_left.yh}});
            vertices.push_back(
                    {{pos.top_left.xh, pos.top_left.yh + pos.extents.height,
                      cmd.z},
                     cmd.colour,
                     {uv.top_left.xh, uv_br.yh}});

            indices.push_back(quad_index);
            indices.push_back(quad_index + 1);
            indices.push_back(quad_index + 2);
            indices.push_back(quad_index);
            indices.push_back(quad_index + 2);
            indices.push_back(quad_index + 3);
        }
    }

    /// #### Upload combined buffers to GPU once
    auto &vertex_buffer = vertex_buffers[rp.current_frame];
    vertex_buffer = {
            rp.renderer.per_frame_memory, vertices,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    auto &index_buffer = index_buffers[rp.current_frame];
    index_buffer = {
            rp.renderer.per_frame_memory, indices,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    std::array buffers{vertex_buffer.get()};
    std::array offset{VkDeviceSize{}};
    vkCmdBindVertexBuffers(
            rp.cb.get(), 0, buffers.size(), buffers.data(), offset.data());
    vkCmdBindIndexBuffer(
            rp.cb.get(), index_buffer.get(), 0, VK_INDEX_TYPE_UINT32);

    /// #### Pass 2
    /// Issue one draw call per texture using offsets into the combined buffers
    std::size_t texture_index = 0;
    std::uint32_t first_index = 0;
    for (auto const &[texture, cmds] : commands.non_empty_vectors()) {
        auto const index_count = static_cast<std::uint32_t>(cmds.size()) * 6u;

        VkDescriptorImageInfo const texture_info{
                .sampler = texture->sampler.get(),
                .imageView = texture->image_view.get(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        VkWriteDescriptorSet wds{};
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        wds.dstSet = textures_ubo.sets[rp.current_frame][texture_index];
        wds.dstBinding = 0;
        wds.dstArrayElement = 0;
        wds.descriptorCount = 1;
        wds.pImageInfo = &texture_info;
        vkUpdateDescriptorSets(
                rp.renderer.app.device.get(), 1, &wds, 0, nullptr);

        vkCmdBindDescriptorSets(
                rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout.get(), 1, 1,
                &textures_ubo.sets[rp.current_frame][texture_index], 0,
                nullptr);

        static constexpr std::uint32_t instance_count = 1;
        static constexpr std::int32_t vertex_offset = 0;
        static constexpr std::uint32_t first_instance = 0;
        vkCmdDrawIndexed(
                rp.cb.get(), index_count, instance_count, first_index,
                vertex_offset, first_instance);

        first_index += index_count;
        ++texture_index;
    }

    commands.clear();
}

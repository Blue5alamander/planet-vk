#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/engine/textured.pipeline.hpp>


/// ## `planet::vk::engine::pipeline::textured`


planet::vk::engine::pipeline::textured::textured(
        std::string_view const n,
        engine::renderer &r,
        std::string_view const vs,
        std::uint32_t const mtpf,
        id::suffix const suffix)
: id{n, suffix},
  textures{r.app.device, mtpf},
  pipeline{planet::vk::engine::create_graphics_pipeline(
          {.app = r.app,
           .renderer = r,
           .vertex_shader = vs,
           .fragment_shader = "planet-vk-engine/textured.frag.spirv",
           .binding_descriptions = textures.binding_description,
           .attribute_descriptions = textures.attribute_description,
           .pipeline_layout =
                   pipeline_layout{
                           r.app.device,
                           std::array{
                                   r.coordinates_ubo_layout().get(),
                                   textures.layout.get()}}})},
  textures_in_frame{name() + "__textures_in_frame"} {}


void planet::vk::engine::pipeline::textured::render(render_parameters rp) {
    if (empty()) { return; }

    auto &vertex_buffer = textures.vertex_buffers[rp.current_frame];
    vertex_buffer = {
            rp.renderer.per_frame_memory, vertices,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    auto &index_buffer = textures.index_buffers[rp.current_frame];
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

    textures_in_frame.value(textures.descriptors.size());
    if (textures.descriptors.size() > textures.max_per_frame) {
        planet::log::error("We will run out of texture slots for this frame");
    }
    for (std::size_t index{}; auto const &tx : textures.descriptors) {
        VkWriteDescriptorSet wds{};
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        wds.dstSet = textures.sets[rp.current_frame][index];
        wds.dstBinding = 0;
        wds.dstArrayElement = 0;
        wds.descriptorCount = 1;
        wds.pImageInfo = &tx;
        vkUpdateDescriptorSets(
                rp.renderer.app.device.get(), 1, &wds, 0, nullptr);

        vkCmdBindDescriptorSets(
                rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.layout.get(), 1, 1,
                &textures.sets[rp.current_frame][index], 0, nullptr);

        static constexpr std::uint32_t index_count = 6;
        static constexpr std::uint32_t instance_count = 1;
        static constexpr std::int32_t vertex_offset = 0;
        static constexpr std::uint32_t first_instance = 0;
        vkCmdDrawIndexed(
                rp.cb.get(), index_count, instance_count, index * index_count,
                vertex_offset, first_instance);

        ++index;
    }

    // Clear out data from this frame
    clear();
}


void planet::vk::engine::pipeline::textured::draw(
        std::span<vk::textures<max_frames_in_flight>::vertex const> const vs,
        std::span<std::uint32_t const> const ix,
        std::span<VkDescriptorImageInfo const> const tx) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back(v); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
    for (auto const &t : tx) { textures.descriptors.push_back(t); }
}


void planet::vk::engine::pipeline::textured::draw(
        vk::sub_texture const &texture,
        affine::rectangle2d const &pos,
        vk::colour const &colour,
        float const z) {
    std::size_t const quad_index = vertices.size();

    vertices.push_back(
            {planet::affine::point3d{pos.bottom_right(), z},
             colour,
             {texture.second.bottom_right().x(),
              texture.second.bottom_right().y()}});
    vertices.push_back(
            {planet::affine::point3d{
                     pos.bottom_right().x(), pos.top_left.y(), z},
             colour,
             {texture.second.bottom_right().x(), texture.second.top_left.y()}});
    vertices.push_back(
            {planet::affine::point3d{pos.top_left, z},
             colour,
             {texture.second.top_left.x(), texture.second.top_left.y()}});
    vertices.push_back(
            {planet::affine::point3d{
                     pos.top_left.x(), pos.bottom_right().y(), z},
             colour,
             {texture.second.top_left.x(), texture.second.bottom_right().y()}});

    indices.push_back(quad_index);
    indices.push_back(quad_index + 2);
    indices.push_back(quad_index + 1);
    indices.push_back(quad_index);
    indices.push_back(quad_index + 3);
    indices.push_back(quad_index + 2);

    textures.descriptors.emplace_back();
    textures.descriptors.back().imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textures.descriptors.back().imageView = texture.first.image_view.get();
    textures.descriptors.back().sampler = texture.first.sampler.get();
}

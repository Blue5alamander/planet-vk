#include <planet/functional.hpp>
#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/engine/pipeline/textured.hpp>


/// ## `planet::vk::engine::pipeline::textured`


planet::vk::engine::pipeline::textured::textured(parameters const p)
: id{p.name, p.use_name_suffix},
  textures{name(), p.renderer.app.device, p.textures_per_frame},
  pipeline{planet::vk::engine::create_graphics_pipeline(
          {.app = p.renderer.app,
           .renderer = p.renderer,
           .vertex_shader = p.vertex_shader,
           .fragment_shader = p.fragment_shader,
           .binding_descriptions =
                   vertex::binding_description<textures_type::vertex_type>(),
           .attribute_descriptions =
                   vertex::attribute_description<textures_type::vertex_type>(),
           .pipeline_layout = pipeline_layout{
                   p.renderer.app.device,
                   std::array{
                           p.renderer.coordinates_ubo_layout().get(),
                           textures.ubo.layout.get()}}})} {}


void planet::vk::engine::pipeline::textured::render(render_parameters rp) {
    if (not textures.bind(
                rp.renderer.per_frame_memory, rp.current_frame, rp.cb)) {
        return;
    }
    planet::by_index(
            textures.descriptors, [&](auto const index, auto const &tx) {
                VkWriteDescriptorSet wds{};
                wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                wds.dstSet = textures.ubo.sets[rp.current_frame][index];
                wds.dstBinding = 0;
                wds.dstArrayElement = 0;
                wds.descriptorCount = 1;
                wds.pImageInfo = &tx;
                vkUpdateDescriptorSets(
                        rp.renderer.app.device.get(), 1, &wds, 0, nullptr);

                vkCmdBindDescriptorSets(
                        rp.cb.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline.layout.get(), 1, 1,
                        &textures.ubo.sets[rp.current_frame][index], 0,
                        nullptr);

                static constexpr std::uint32_t index_count = 6;
                static constexpr std::uint32_t instance_count = 1;
                static constexpr std::int32_t vertex_offset = 0;
                static constexpr std::uint32_t first_instance = 0;
                vkCmdDrawIndexed(
                        rp.cb.get(), index_count, instance_count,
                        index * index_count, vertex_offset, first_instance);
            });

    // Clear out data from this frame
    textures.clear();
}


void planet::vk::engine::pipeline::textured::draw(
        vk::sub_texture const &texture,
        affine::rectangle2d const &pos,
        planet::colour const &colour,
        float const z) {
    std::size_t const quad_index = textures.vertices.size();

    textures.vertices.push_back(
            {planet::affine::point3d{pos.bottom_right(), z},
             colour,
             {texture.second.bottom_right().x(),
              texture.second.bottom_right().y()}});
    textures.vertices.push_back(
            {planet::affine::point3d{
                     pos.bottom_right().x(), pos.top_left.y(), z},
             colour,
             {texture.second.bottom_right().x(), texture.second.top_left.y()}});
    textures.vertices.push_back(
            {planet::affine::point3d{pos.top_left, z},
             colour,
             {texture.second.top_left.x(), texture.second.top_left.y()}});
    textures.vertices.push_back(
            {planet::affine::point3d{
                     pos.top_left.x(), pos.bottom_right().y(), z},
             colour,
             {texture.second.top_left.x(), texture.second.bottom_right().y()}});

    textures.indices.push_back(quad_index);
    textures.indices.push_back(quad_index + 1);
    textures.indices.push_back(quad_index + 2);
    textures.indices.push_back(quad_index);
    textures.indices.push_back(quad_index + 2);
    textures.indices.push_back(quad_index + 3);

    textures.descriptors.emplace_back();
    textures.descriptors.back().imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textures.descriptors.back().imageView = texture.first.image_view.get();
    textures.descriptors.back().sampler = texture.first.sampler.get();
}

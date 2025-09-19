#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/engine/sprite.pipeline.hpp>


/// ## `planet::vk::engine::pipeline::sprite`


planet::vk::engine::pipeline::sprite::sprite(
        std::string_view const n,
        engine::renderer &r,
        std::string_view const vs,
        std::uint32_t const mtpf,
        id::suffix const suffix)
: id{n, suffix},
  textures{name(), r.app.device, mtpf},
  pipeline{create_pipeline(r, vs)} {}


planet::vk::graphics_pipeline
        planet::vk::engine::pipeline::sprite::create_pipeline(
                engine::renderer &r, std::string_view const vertex_shader) {
    VkPushConstantRange pc;
    pc.offset = 0;
    pc.size = sizeof(push_constant);
    pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    return planet::vk::engine::create_graphics_pipeline(
            {.app = r.app,
             .renderer = r,
             .vertex_shader = {vertex_shader},
             .fragment_shader = {"planet-vk-engine/sprite.frag.spirv"},
             .binding_descriptions =
                     vertex::binding_description<textures_type::vertex_type>(),
             .attribute_descriptions =
                     vertex::attribute_description<textures_type::vertex_type>(),
             .pipeline_layout = pipeline_layout{
                     r.app.device,
                     std::array{
                             r.coordinates_ubo_layout().get(),
                             textures.layout.get()},
                     std::array{pc}}});
}


void planet::vk::engine::pipeline::sprite::draw(
        std::pair<vk::texture const &, affine::rectangle2d> texture,
        location const &loc,
        vk::colour const &colour) {
    auto const pos = affine::rectangle2d{
            affine::point2d{0, loc.size.height} - loc.centre,
            affine::extents2d{loc.size.width, -loc.size.height}};
    std::size_t const quad_index = textures.vertices.size();

    textures.vertices.push_back(
            {{pos.bottom_right().x(), pos.bottom_right().y(), loc.z_height},
             colour,
             {texture.second.bottom_right().x(),
              texture.second.bottom_right().y()}});
    textures.vertices.push_back(
            {{pos.bottom_right().x(), pos.top_left.y(), loc.z_height},
             colour,
             {texture.second.bottom_right().x(), texture.second.top_left.y()}});
    textures.vertices.push_back(
            {{pos.top_left.x(), pos.top_left.y(), loc.z_height},
             colour,
             {texture.second.top_left.x(), texture.second.top_left.y()}});
    textures.vertices.push_back(
            {{pos.top_left.x(), pos.bottom_right().y(), loc.z_height},
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

    transforms.push_back({planet::affine::matrix3d{
            planet::affine::matrix2d::translate(
                    {loc.offset.xh, loc.offset.yh, loc.offset.h})
            * planet::affine::matrix2d::rotate(loc.rotation)}});
}


void planet::vk::engine::pipeline::sprite::render(render_parameters rp) {
    if (not textures.bind(
                rp.renderer.per_frame_memory, rp.current_frame, rp.cb)) {
        return;
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

        vkCmdPushConstants(
                rp.cb.get(), pipeline.layout.get(), VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(push_constant), &transforms[index]);

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
    textures.clear();
    transforms.clear();
}

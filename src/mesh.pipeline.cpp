#include <planet/telemetry/counter.hpp>
#include <planet/vk/engine/pipeline/mesh.hpp>
#include <planet/vk/engine/renderer.hpp>


/// ## `planet::vk::engine::pipeline::mesh`


planet::vk::engine::pipeline::mesh::mesh(parameters p)
: pipeline{planet::vk::engine::create_graphics_pipeline(
          {.app = p.renderer.app,
           .renderer = p.renderer,
           .vertex_shader = p.vertex_shader,
           .fragment_shader = p.fragment_shader,
           .binding_descriptions =
                   vertex::binding_description<vertex::coloured>(),
           .attribute_descriptions =
                   vertex::attribute_description<vertex::coloured>(),
           .blend_mode = p.blend_mode,
           .pipeline_layout = std::move(p.layout)})} {}


planet::vk::pipeline_layout planet::vk::engine::pipeline::mesh::default_layout(
        engine::renderer &r) {
    return pipeline_layout{r.app.device, r.coordinates_ubo_layout()};
}


namespace {
    planet::telemetry::counter vertex_count{
            "planet_vk_engine_pipeline_mesh_render_vertices"};
    planet::telemetry::counter index_count{
            "planet_vk_engine_pipeline_mesh_render_indices"};
}
void planet::vk::engine::pipeline::mesh::render(render_parameters rp) {
    if (this_frame.empty()) { return; }

    vertex_count += this_frame.vertices.size();
    index_count += this_frame.indices.size();

    auto &vertex_buffer = vertex_buffers[rp.current_frame];
    vertex_buffer = {
            rp.renderer.per_frame_memory, this_frame.vertices,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    auto &index_buffer = index_buffers[rp.current_frame];
    index_buffer = {
            rp.renderer.per_frame_memory, this_frame.indices,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    std::array buffers{vertex_buffer.get()};
    std::array offset{VkDeviceSize{}};

    vkCmdBindVertexBuffers(
            rp.cb.get(), 0, buffers.size(), buffers.data(), offset.data());
    vkCmdBindIndexBuffer(
            rp.cb.get(), index_buffer.get(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(
            rp.cb.get(), static_cast<uint32_t>(this_frame.indices.size()), 1, 0,
            0, 0);

    this_frame.clear();
}


void planet::vk::engine::pipeline::mesh::draw(data const &d) {
    /// TODO This should really end up as its own GPU upload
    this_frame.draw(d.vertices, d.indices);
}


/// ## `planet::vk::engine::pipeline::mesh::data`


void planet::vk::engine::pipeline::mesh::data::draw(
        std::span<vertex::coloured const> const vs,
        std::span<std::uint32_t const> const ix) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back(v); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
}
void planet::vk::engine::pipeline::mesh::data::draw(
        std::span<vertex::coloured const> const vs,
        std::span<std::uint32_t const> const ix,
        planet::affine::point3d const &p) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back({v.p + p, v.col}); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
}
void planet::vk::engine::pipeline::mesh::data::draw(
        std::span<vertex::coloured const> const vs,
        std::span<std::uint32_t const> const ix,
        planet::affine::point3d const &p,
        colour const &c) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back({v.p + p, c}); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
}

#include <planet/telemetry/counter.hpp>
#include <planet/vk/engine/mesh.pipeline.hpp>
#include <planet/vk/engine/renderer.hpp>


/// ## `planet::vk::engine::pipeline::mesh`


namespace {


    constexpr auto binding_description{[]() {
        VkVertexInputBindingDescription description{};

        description.binding = 0;
        description.stride = sizeof(planet::vk::engine::pipeline::mesh::vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::array{description};
    }()};
    constexpr auto attribute_description{[]() {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[0].offset =
                offsetof(planet::vk::engine::pipeline::mesh::vertex, p);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[1].offset =
                offsetof(planet::vk::engine::pipeline::mesh::vertex, c);

        return attrs;
    }()};


}


planet::vk::engine::pipeline::mesh::mesh(
        engine::renderer &r,
        std::string_view const vertex_spirv_filename,
        std::string_view const fragment_spirv_filename,
        blend_mode const bm,
        pipeline_layout layout)
: pipeline{planet::vk::engine::create_graphics_pipeline(
          {.app = r.app,
           .renderer = r,
           .vertex_shader = vertex_spirv_filename,
           .fragment_shader = fragment_spirv_filename,
           .binding_descriptions = binding_description,
           .attribute_descriptions = attribute_description,
           .blend_mode = bm,
           .pipeline_layout = std::move(layout)})} {}


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
        std::span<vertex const> const vs,
        std::span<std::uint32_t const> const ix) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back(v); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
}
void planet::vk::engine::pipeline::mesh::data::draw(
        std::span<vertex const> const vs,
        std::span<std::uint32_t const> const ix,
        planet::affine::point3d const &p) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back({v.p + p, v.c}); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
}
void planet::vk::engine::pipeline::mesh::data::draw(
        std::span<vertex const> const vs,
        std::span<std::uint32_t const> const ix,
        planet::affine::point3d const &p,
        colour const &c) {
    auto const start_index = vertices.size();
    for (auto const &v : vs) { vertices.push_back({v.p + p, c}); }
    for (auto const &i : ix) { indices.push_back(start_index + i); }
}

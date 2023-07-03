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
        attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
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
        engine::app &a, engine::renderer &r, blend_mode const bm)
: mesh{a, r.swap_chain, r.render_pass, r.ubo_layout, bm} {}


planet::vk::engine::pipeline::mesh::mesh(
        engine::app &a,
        engine::renderer &r,
        std::string_view const vertex_spirv_filename,
        blend_mode const bm)
: mesh{a, r.swap_chain, r.render_pass, r.ubo_layout, vertex_spirv_filename,
       bm} {}


planet::vk::engine::pipeline::mesh::mesh(
        engine::app &a,
        vk::swap_chain &sc,
        vk::render_pass &rp,
        vk::descriptor_set_layout &dsl,
        blend_mode const bm)
: mesh{a, sc, rp, dsl, "planet-vk-engine/mesh.world.vert.spirv", bm} {}


planet::vk::engine::pipeline::mesh::mesh(
        engine::app &a,
        vk::swap_chain &sc,
        vk::render_pass &rp,
        vk::descriptor_set_layout &dsl,
        std::string_view const vertex_spirv_filename,
        blend_mode const bm)
: app{a},
  swap_chain{sc},
  render_pass{rp},
  ubo_layout{dsl},
  pipeline{create_mesh_pipeline(vertex_spirv_filename, bm)} {}


planet::vk::graphics_pipeline
        planet::vk::engine::pipeline::mesh::create_mesh_pipeline(
                std::string_view const vert_spirv_filename,
                blend_mode const bm) {
    return planet::vk::engine::create_graphics_pipeline(
            app, vert_spirv_filename, "planet-vk-engine/mesh.frag.spirv",
            binding_description, attribute_description, swap_chain, render_pass,
            pipeline_layout{app.device, ubo_layout}, bm);
}


void planet::vk::engine::pipeline::mesh::draw(
        std::span<vertex const> const vertices,
        std::span<std::uint32_t const> const indices) {
    auto const start_index = triangles.size();
    for (auto const &v : vertices) { triangles.push_back(v); }
    for (auto const &i : indices) { indexes.push_back(start_index + i); }
}
void planet::vk::engine::pipeline::mesh::draw(
        std::span<vertex const> const vertices,
        std::span<std::uint32_t const> const indices,
        pos const p) {
    auto const start_index = triangles.size();
    for (auto const &v : vertices) { triangles.push_back({v.p + p, v.c}); }
    for (auto const &i : indices) { indexes.push_back(start_index + i); }
}
void planet::vk::engine::pipeline::mesh::draw(
        std::span<vertex const> const vertices,
        std::span<std::uint32_t const> const indices,
        pos const p,
        colour const &c) {
    auto const start_index = triangles.size();
    for (auto const &v : vertices) { triangles.push_back({v.p + p, c}); }
    for (auto const &i : indices) { indexes.push_back(start_index + i); }
}


void planet::vk::engine::pipeline::mesh::render(render_parameters rp) {
    if (triangles.empty()) { return; }

    auto &vertex_buffer = vertex_buffers[rp.current_frame];
    vertex_buffer = {
            rp.renderer.per_frame_memory, triangles,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    auto &index_buffer = index_buffers[rp.current_frame];
    index_buffer = {
            rp.renderer.per_frame_memory, indexes,
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
            rp.cb.get(), static_cast<uint32_t>(indexes.size()), 1, 0, 0, 0);

    triangles.clear();
    indexes.clear();
}

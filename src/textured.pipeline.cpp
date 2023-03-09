#include <planet/vk/engine2d/renderer.hpp>
#include <planet/vk/engine2d/textured.pipeline.hpp>


/// ## `planet::vk::engine2d::pipeline::textured`


namespace {


    template<typename V>
    auto binding_description();

    template<typename V>
    auto attribute_description();

    template<>
    auto binding_description<planet::vk::engine2d::pipeline::textured::vertex>() {
        VkVertexInputBindingDescription description{};

        description.binding = 0;
        description.stride =
                sizeof(planet::vk::engine2d::pipeline::textured::vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::array{description};
    }

    template<>
    auto attribute_description<
            planet::vk::engine2d::pipeline::textured::vertex>() {
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
    }


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
    planet::vk::shader_module vertex_shader_module{
            app.device,
            app.asset_manager.file_data(
                    "planet-vk-engine2d/textured.vert.spirv")};
    planet::vk::shader_module fragment_shader_module{
            app.device,
            app.asset_manager.file_data(
                    "planet-vk-engine2d/textured.frag.spirv")};
    std::array shader_stages{
            vertex_shader_module.shader_stage_info(
                    VK_SHADER_STAGE_VERTEX_BIT, "main"),
            fragment_shader_module.shader_stage_info(
                    VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

    /// Vertex bindings and attributes
    auto const binding = binding_description<vertex>();
    auto const attrs = attribute_description<vertex>();
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = binding.size();
    vertex_input_info.pVertexBindingDescriptions = binding.data();
    vertex_input_info.vertexAttributeDescriptionCount = attrs.size();
    vertex_input_info.pVertexAttributeDescriptions = attrs.data();

    // Primitive type
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport config
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = app.window.zwidth();
    viewport.height = app.window.zheight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor rect config
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swap_chain().extents;

    VkPipelineViewportStateCreateInfo viewport_state_info = {};
    viewport_state_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
    rasterizer_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.lineWidth = 1.f;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blend_mode = {};
    blend_mode.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    blend_mode.blendEnable = VK_TRUE;
    blend_mode.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_mode.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_mode.colorBlendOp = VK_BLEND_OP_ADD;
    blend_mode.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_mode.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_mode.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.logicOpEnable = VK_FALSE;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_mode;

    VkGraphicsPipelineCreateInfo graphics_pipeline_info = {};
    graphics_pipeline_info.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.stageCount = 2;
    graphics_pipeline_info.pStages = shader_stages.data();
    graphics_pipeline_info.pVertexInputState = &vertex_input_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly;
    graphics_pipeline_info.pViewportState = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &rasterizer_info;
    graphics_pipeline_info.pMultisampleState = &multisampling;
    graphics_pipeline_info.pColorBlendState = &blend_info;
    graphics_pipeline_info.subpass = 0;

    std::array layouts{vp_layout.get(), texture_layout.get()};
    return planet::vk::graphics_pipeline{
            app.device, graphics_pipeline_info, render_pass,
            planet::vk::pipeline_layout{app.device, layouts}};
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

    // For now swap the vertex order so we know we'll see at least one of them
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

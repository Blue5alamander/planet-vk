#include <planet/asset_manager.hpp>
#include <planet/vk.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "viking_room.obj";
const std::string TEXTURE_PATH = "viking_room.png";

const int MAX_FRAMES_IN_FLIGHT = 2;


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3>
            getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex &other) const {
        return pos == other.pos && color == other.color
                && texCoord == other.texCoord;
    }
};

namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const &vertex) const {
            return ((hash<glm::vec3>()(vertex.pos)
                     ^ (hash<glm::vec3>()(vertex.color) << 1))
                    >> 1)
                    ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class HelloTriangleApplication {
    planet::asset_manager am;

  public:
    HelloTriangleApplication(int const, char const *const argv[])
    : am{argv[0]} {}

    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

  private:
    GLFWwindow *window = [this]() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        auto *w = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(w, this);
        glfwSetFramebufferSizeCallback(w, framebufferResizeCallback);
        return w;
    }();

    planet::vk::extensions extensions;

    planet::vk::instance instance = [this]() {
        auto appInfo = planet::vk::application_info();
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

        getRequiredExtensions();
        auto createInfo = planet::vk::instance::info(extensions, appInfo);

        if (not checkValidationLayerSupport()) {
            throw std::runtime_error(
                    "validation layers requested, but not available!");
        }

        return planet::vk::instance{
                extensions, createInfo, [this](VkInstance h) {
                    VkSurfaceKHR surface = VK_NULL_HANDLE;
                    planet::vk::worked(glfwCreateWindowSurface(
                            h, window, nullptr, &surface));
                    return surface;
                }};
    }();

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    planet::vk::device device = [this]() {
        pickPhysicalDevice();
        return planet::vk::device{instance, extensions};
    }();

    planet::vk::swap_chain swapChain{device, chooseSwapExtent()};

    planet::vk::descriptor_set_layout descriptorSetLayout = [this]() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
                uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        return planet::vk::descriptor_set_layout{device, layoutInfo};
    }();

    planet::vk::graphics_pipeline graphicsPipeline{[this]() {
        planet::vk::shader_module vertShaderModule{
                device, am.file_data("27_shader_depth.vert.spirv")};
        planet::vk::shader_module fragShaderModule{
                device, am.file_data("27_shader_depth.frag.spirv")};
        std::array shaderStages{
                vertShaderModule.shader_stage_info(
                        VK_SHADER_STAGE_VERTEX_BIT, "main"),
                fragShaderModule.shader_stage_info(
                        VK_SHADER_STAGE_FRAGMENT_BIT, "main")};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions =
                attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType =
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = msaaSamples;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType =
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount =
                static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayout.address();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        return planet::vk::graphics_pipeline(
                device, pipelineInfo, {[this]() {
                    VkAttachmentDescription colorAttachment{};
                    colorAttachment.format = swapChain.image_format;
                    colorAttachment.samples = msaaSamples;
                    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    colorAttachment.stencilLoadOp =
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    colorAttachment.stencilStoreOp =
                            VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    colorAttachment.finalLayout =
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    VkAttachmentDescription depthAttachment{};
                    depthAttachment.format = findDepthFormat();
                    depthAttachment.samples = msaaSamples;
                    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    depthAttachment.stencilLoadOp =
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    depthAttachment.stencilStoreOp =
                            VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    depthAttachment.finalLayout =
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                    VkAttachmentDescription colorAttachmentResolve{};
                    colorAttachmentResolve.format = swapChain.image_format;
                    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
                    colorAttachmentResolve.loadOp =
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    colorAttachmentResolve.storeOp =
                            VK_ATTACHMENT_STORE_OP_STORE;
                    colorAttachmentResolve.stencilLoadOp =
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    colorAttachmentResolve.stencilStoreOp =
                            VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    colorAttachmentResolve.initialLayout =
                            VK_IMAGE_LAYOUT_UNDEFINED;
                    colorAttachmentResolve.finalLayout =
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                    VkAttachmentReference colorAttachmentRef{};
                    colorAttachmentRef.attachment = 0;
                    colorAttachmentRef.layout =
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    VkAttachmentReference depthAttachmentRef{};
                    depthAttachmentRef.attachment = 1;
                    depthAttachmentRef.layout =
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                    VkAttachmentReference colorAttachmentResolveRef{};
                    colorAttachmentResolveRef.attachment = 2;
                    colorAttachmentResolveRef.layout =
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    VkSubpassDescription subpass{};
                    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                    subpass.colorAttachmentCount = 1;
                    subpass.pColorAttachments = &colorAttachmentRef;
                    subpass.pDepthStencilAttachment = &depthAttachmentRef;
                    subpass.pResolveAttachments = &colorAttachmentResolveRef;

                    VkSubpassDependency dependency{};
                    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                    dependency.dstSubpass = 0;
                    dependency.srcStageMask =
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    dependency.srcAccessMask = 0;
                    dependency.dstStageMask =
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    dependency.dstAccessMask =
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                    std::array attachments{
                            colorAttachment, depthAttachment,
                            colorAttachmentResolve};
                    VkRenderPassCreateInfo renderPassInfo{};
                    renderPassInfo.sType =
                            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                    renderPassInfo.attachmentCount =
                            static_cast<uint32_t>(attachments.size());
                    renderPassInfo.pAttachments = attachments.data();
                    renderPassInfo.subpassCount = 1;
                    renderPassInfo.pSubpasses = &subpass;
                    renderPassInfo.dependencyCount = 1;
                    renderPassInfo.pDependencies = &dependency;

                    return planet::vk::render_pass{device, renderPassInfo};
                }()},
                {device, pipelineLayoutInfo});
    }()};

    planet::vk::command_pool commandPool{device, instance.surface};

    planet::vk::image colorImage;
    planet::vk::image_view colorImageView;

    planet::vk::image depthImage;
    planet::vk::image_view depthImageView;

    uint32_t mipLevels;
    planet::vk::image textureImage;
    planet::vk::image_view textureImageView;
    planet::vk::sampler textureSampler;

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    planet::vk::buffer<Vertex> vertexBuffer;
    planet::vk::buffer<std::uint32_t> indexBuffer;

    std::vector<planet::vk::buffer<UniformBufferObject>> uniformBuffers;
    std::vector<planet::vk::device_memory::mapping> uniformBuffersMapped;

    planet::vk::descriptor_pool descriptorPool = [this]() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount =
                static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount =
                static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        return planet::vk::descriptor_pool{
                device, poolSizes, MAX_FRAMES_IN_FLIGHT};
    }();
    planet::vk::descriptor_sets descriptorSets{
            descriptorPool, descriptorSetLayout, MAX_FRAMES_IN_FLIGHT};

    planet::vk::command_buffers commandBuffers{
            commandPool, MAX_FRAMES_IN_FLIGHT};

    std::vector<planet::vk::semaphore> imageAvailableSemaphores;
    std::vector<planet::vk::semaphore> renderFinishedSemaphores;
    std::vector<planet::vk::fence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    static void framebufferResizeCallback(
            GLFWwindow *window, int /*width*/, int /*height*/) {
        auto app = reinterpret_cast<HelloTriangleApplication *>(
                glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        createSwapChain();
        createColorResources();
        createDepthResources();
        swapChain.create_frame_buffers(
                graphicsPipeline.render_pass, colorImageView.get(),
                depthImageView.get());
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorSets();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        device.wait_idle();
    }

    void cleanup() {
        uniformBuffersMapped.clear();
        uniformBuffers.clear();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.wait_idle();

        createSwapChain();
        createColorResources();
        createDepthResources();
        swapChain.create_frame_buffers(
                graphicsPipeline.render_pass, colorImageView.get(),
                depthImageView.get());
    }

    void pickPhysicalDevice() {
        auto const devices = instance.physical_devices();

        for (auto const &device : devices) {
            if (isDeviceSuitable(device)) {
                instance.use_gpu(device);
                msaaSamples = getMaxUsableSampleCount();
                return;
            }
        }
        throw felspar::stdexcept::runtime_error{
                "failed to find a suitable GPU!"};
    }

    void createSwapChain() {
        instance.surface.refresh_characteristics(instance.gpu());
        VkExtent2D extent = chooseSwapExtent();
        swapChain.recreate(extent);
    }

    void createColorResources() {
        VkFormat colorFormat = swapChain.image_format;

        colorImage = {
                device.startup_memory,
                swapChain.extents.width,
                swapChain.extents.height,
                1,
                msaaSamples,
                colorFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        colorImageView = {
                colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1};
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        depthImage = {
                device.startup_memory,
                swapChain.extents.width,
                swapChain.extents.height,
                1,
                msaaSamples,
                depthFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        depthImageView = {
                depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1};
    }

    VkFormat findSupportedFormat(
            const std::vector<VkFormat> &candidates,
            VkImageTiling tiling,
            VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(
                    instance.gpu().get(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR
                && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (
                    tiling == VK_IMAGE_TILING_OPTIMAL
                    && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                 VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT
                || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        auto const texdata = am.file_data(TEXTURE_PATH);
        stbi_uc *pixels = stbi_load_from_memory(
                reinterpret_cast<stbi_uc const *>(texdata.data()),
                texdata.size(), &texWidth, &texHeight, &texChannels,
                STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        mipLevels = static_cast<uint32_t>(std::floor(
                            std::log2(std::max(texWidth, texHeight))))
                + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        planet::vk::buffer<std::byte> stagingBuffer{
                device.staging_memory, imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        auto stagingBufferMemory{stagingBuffer.map()};
        memcpy(stagingBufferMemory.get(), pixels,
               static_cast<size_t>(imageSize));

        stbi_image_free(pixels);

        textureImage = {
                device.startup_memory,
                static_cast<uint32_t>(texWidth),
                static_cast<uint32_t>(texHeight),
                mipLevels,
                VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                        | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        transitionImageLayout(
                textureImage.get(), VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                mipLevels);
        copyBufferToImage(
                stagingBuffer.get(), textureImage.get(),
                static_cast<uint32_t>(texWidth),
                static_cast<uint32_t>(texHeight));
        // transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while
        // generating mipmaps

        generateMipmaps(
                textureImage.get(), VK_FORMAT_R8G8B8A8_SRGB, texWidth,
                texHeight, mipLevels);
    }

    void generateMipmaps(
            VkImage image,
            VkFormat imageFormat,
            int32_t texWidth,
            int32_t texHeight,
            uint32_t mipLevels) {
        // Check if image format supports linear blitting
        auto const formatProperties =
                instance.gpu().format_properties(imageFormat);

        if (!(formatProperties.optimalTilingFeatures
              & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error(
                    "texture image format does not support linear blitting!");
        }

        auto commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                    commandBuffer.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                    1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                    mipWidth > 1 ? mipWidth / 2 : 1,
                    mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(
                    commandBuffer.get(), image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                    VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                    commandBuffer.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                    nullptr, 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
                commandBuffer.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        endSingleTimeCommands(std::move(commandBuffer));
    }

    VkSampleCountFlagBits getMaxUsableSampleCount() {
        VkSampleCountFlags counts =
                instance.gpu().properties.limits.framebufferColorSampleCounts
                & instance.gpu().properties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    void createTextureImageView() {
        textureImageView = {
                textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_ASPECT_COLOR_BIT, mipLevels};
    }

    void createTextureSampler() { textureSampler = {device, mipLevels}; }

    void transitionImageLayout(
            VkImage image,
            VkFormat,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels) {
        auto commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (
                oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
                commandBuffer.get(), sourceStage, destinationStage, 0, 0,
                nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(std::move(commandBuffer));
    }

    void copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        auto commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(
                commandBuffer.get(), buffer, image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(std::move(commandBuffer));
    }

    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        auto const objdata = am.file_data(MODEL_PATH);
        std::istringstream stream{std::string{
                reinterpret_cast<char const *>(objdata.data()), objdata.size()}};

        if (!tinyobj::LoadObj(
                    &attrib, &shapes, &materials, &warn, &err, &stream)) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]};

                vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] =
                            static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        planet::vk::buffer<Vertex> stagingBuffer{
                device.staging_memory, vertices.size(),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        auto data = stagingBuffer.map();
        memcpy(data.get(), vertices.data(), (size_t)bufferSize);

        vertexBuffer = {
                device.startup_memory, vertices.size(),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT
                        | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        copyBuffer(stagingBuffer.get(), vertexBuffer.get(), bufferSize);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        planet::vk::buffer<std::uint32_t> stagingBuffer{
                device.staging_memory, indices.size(),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        auto data = stagingBuffer.map();
        memcpy(data.get(), indices.data(), (size_t)bufferSize);

        indexBuffer = {
                device.startup_memory, indices.size(),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT
                        | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        copyBuffer(stagingBuffer.get(), indexBuffer.get(), bufferSize);
    }

    void createUniformBuffers() {
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            uniformBuffers[i] = {
                    device.startup_memory, 1,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
            uniformBuffersMapped[i] = uniformBuffers[i].map();
        }
    }

    void createDescriptorSets() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].get();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView.get();
            imageInfo.sampler = textureSampler.get();

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType =
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(
                    device.get(),
                    static_cast<uint32_t>(descriptorWrites.size()),
                    descriptorWrites.data(), 0, nullptr);
        }
    }

    planet::vk::command_buffer beginSingleTimeCommands() {
        planet::vk::command_buffer commandBuffer{commandPool};
        commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        return commandBuffer;
    }

    void endSingleTimeCommands(planet::vk::command_buffer commandBuffer) {
        commandBuffer.end();
        commandBuffer.submit(device.graphics_queue);
        vkQueueWaitIdle(device.graphics_queue);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        auto commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(
                commandBuffer.get(), srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(std::move(commandBuffer));
    }

    void recordCommandBuffer(
            planet::vk::command_buffer &commandBuffer, uint32_t imageIndex) {
        commandBuffer.begin();

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = graphicsPipeline.render_pass.get();
        renderPassInfo.framebuffer = swapChain.frame_buffers[imageIndex].get();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.extents;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount =
                static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(
                commandBuffer.get(), &renderPassInfo,
                VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
                commandBuffer.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.get());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChain.extents.width;
        viewport.height = (float)swapChain.extents.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer.get(), 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChain.extents;
        vkCmdSetScissor(commandBuffer.get(), 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer.get()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(
                commandBuffer.get(), 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(
                commandBuffer.get(), indexBuffer.get(), 0,
                VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(
                commandBuffer.get(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.layout.get(), 0, 1,
                &descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(
                commandBuffer.get(), static_cast<uint32_t>(indices.size()), 1,
                0, 0, 0);

        vkCmdEndRenderPass(commandBuffer.get());

        commandBuffer.end();
    }

    void createSyncObjects() {
        imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores.emplace_back(device);
            renderFinishedSemaphores.emplace_back(device);
            inFlightFences.emplace_back(device);
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(
                             currentTime - startTime)
                             .count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(
                glm::mat4(1.0f), time * glm::radians(90.0f),
                glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(
                glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(
                glm::radians(45.0f),
                swapChain.extents.width / (float)swapChain.extents.height, 0.1f,
                10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage].get(), &ubo, sizeof(ubo));
    }

    void drawFrame() {
        std::array fences{inFlightFences[currentFrame].get()};
        planet::vk::worked(vkWaitForFences(
                device.get(), fences.size(), fences.data(), VK_TRUE,
                UINT64_MAX));

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
                device.get(), swapChain.get(), UINT64_MAX,
                imageAvailableSemaphores[currentFrame].get(), VK_NULL_HANDLE,
                &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame);

        vkResetFences(device.get(), fences.size(), fences.data());

        vkResetCommandBuffer(
                commandBuffers[currentFrame].get(),
                /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        std::array waitSemaphores{imageAvailableSemaphores[currentFrame].get()};
        VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages;

        std::array frameCommandBuffers{commandBuffers[currentFrame].get()};
        submitInfo.commandBufferCount = frameCommandBuffers.size();
        submitInfo.pCommandBuffers = frameCommandBuffers.data();

        std::array signalSemaphores{
                renderFinishedSemaphores[currentFrame].get()};
        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        planet::vk::worked(vkQueueSubmit(
                device.graphics_queue, 1, &submitInfo,
                inFlightFences[currentFrame].get()));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = signalSemaphores.size();
        presentInfo.pWaitSemaphores = signalSemaphores.data();

        std::array swapChains{swapChain.get()};
        presentInfo.swapchainCount = swapChains.size();
        presentInfo.pSwapchains = swapChains.data();

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(device.present_queue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR
            || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkExtent2D chooseSwapExtent() {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        return planet::vk::swap_chain::calculate_extents(
                device,
                {static_cast<float>(width), static_cast<float>(height)});
    }

    bool isDeviceSuitable(planet::vk::physical_device const &device) {
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        /// Make sure that the characteristics we're looking at are for this device
        instance.surface.refresh_characteristics(device);

        bool const swapChainAdequate =
                instance.surface.has_adequate_swap_chain_support();

        return instance.surface.has_queue_families() and extensionsSupported
                and swapChainAdequate and device.features.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(planet::vk::physical_device const &device) {
        auto const availableExtensions = planet::vk::fetch_vector<
                vkEnumerateDeviceExtensionProperties, VkExtensionProperties>(
                device.get(), nullptr);

        std::set<std::string> requiredExtensions(
                extensions.device_extensions.begin(),
                extensions.device_extensions.end());

        for (const auto &extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        extensions.vulkan_extensions.insert(
                extensions.vulkan_extensions.end(), glfwExtensions,
                glfwExtensions + glfwExtensionCount);
    }

    bool checkValidationLayerSupport() {
        auto availableLayers = planet::vk::fetch_vector<
                vkEnumerateInstanceLayerProperties, VkLayerProperties>();

        for (const char *layerName : extensions.validation_layers) {
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) { return false; }
        }

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT,
            VkDebugUtilsMessageTypeFlagsEXT,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *) {
        std::cerr << "validation layer: " << pCallbackData->pMessage
                  << std::endl;

        return VK_FALSE;
    }
};

int main(int argc, char const *argv[]) {
    HelloTriangleApplication app{argc, argv};

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

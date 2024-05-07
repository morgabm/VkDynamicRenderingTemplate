
#include "dynamic_rendering.h"


std::pair<VkInstance, vkb::Instance> init_instance() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Vulkan Test")
                            .request_validation_layers()
                            .use_default_debug_messenger()
                            .require_api_version(1, 3, 0)
                            .build();
    return {inst_ret.value().instance, inst_ret.value()};
}

std::pair<VkPhysicalDevice, vkb::PhysicalDevice> init_physical_device(const vkb::Instance& instance, const VkSurfaceKHR& surface) {
    VkPhysicalDeviceVulkan13Features features13 {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    // Create the physical device
    vkb::PhysicalDeviceSelector selector{instance};
    auto phys_ret = selector.set_surface(surface)
                            .set_minimum_version(1, 3)
                            .set_required_features_13(features13)
                            .select();

    return {phys_ret.value().physical_device, phys_ret.value()};
}

std::pair<VkDevice, vkb::Device> init_device(const vkb::PhysicalDevice& physical_device) {
    vkb::DeviceBuilder device_builder{physical_device};
    auto device_ret = device_builder.build();
    return {device_ret.value().device, device_ret.value()};
}

std::tuple<VkSwapchainKHR, std::vector<VkImage>, std::vector<VkImageView>, vkb::Swapchain> init_swapchain(const vkb::Device& device) {
    vkb::SwapchainBuilder swapchain_builder{device};
    auto                  swapchain_ret = swapchain_builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                 .build();
    auto swapchain = swapchain_ret.value();
    return {swapchain.swapchain, swapchain.get_images().value(), swapchain.get_image_views().value(), swapchain};
}

std::pair<VkCommandPool, VkCommandBuffer> create_primary_command_resources(const DynamicRenderingContext& context) {
    VkCommandPool graphicsCmdPool;
    VkCommandBuffer graphicsPrimaryCmdBuffer;

    VkCommandPoolCreateInfo cmdPoolInfo {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = context.vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    vkCreateCommandPool(context.device, &cmdPoolInfo, nullptr, &graphicsCmdPool);

    VkCommandBufferAllocateInfo cmdBufferInfo {};
    cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount = 1;
    cmdBufferInfo.commandPool = graphicsCmdPool;
    vkAllocateCommandBuffers(context.device, &cmdBufferInfo, &graphicsPrimaryCmdBuffer);

    return {graphicsCmdPool, graphicsPrimaryCmdBuffer};
}

std::vector<uint32_t> readFile(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        exit(-1);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<uint32_t> code(size / 4);
    fread(code.data(), 4, size / 4, file);
    fclose(file);

    return code;
}

VkShaderModule create_shader_module(const DynamicRenderingContext& context, const char* filename) {
    auto code = readFile(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        abort();
    }

    return shaderModule;
}

DynamicRenderingPipeline create_dynamic_rendering_pipeline(const DynamicRenderingContext& context, std::vector<VkPipelineShaderStageCreateInfo> shaderStages) {
    DynamicRenderingPipeline pipeline;
    pipeline.formats = { context.vkb_swapchain.image_format , VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;
    multisampling.pSampleMask           = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable      = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = VK_FALSE;
    depthStencil.depthWriteEnable      = VK_FALSE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.maxDepthBounds        = 1.0f;
    depthStencil.minDepthBounds        = 0.0f;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipeline.layout);

    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRenderingCreateInfoKHR renderingCreateInfo {};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &pipeline.formats[0];
    renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext               = &renderingCreateInfo;
    pipelineInfo.stageCount          = shaderStages.size();
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = pipeline.layout;
    pipelineInfo.renderPass          = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline);

    return pipeline;
}
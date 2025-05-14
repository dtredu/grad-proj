
#include "types.hpp"
#include <vulkan/vulkan_core.h>


void Pipeline::destroyPipelineLayout() {
    vkDestroyPipelineLayout(this->device->device, this->pipelineLayout, nullptr);
}
void Pipeline::createPipelineLayout() {

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 128;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (VK_SUCCESS != vkCreatePipelineLayout(this->device->device, &pipelineLayoutInfo, nullptr, &(this->pipelineLayout))) { 
        throw std::runtime_error("failed to enumerate phdev extensions");
    }
}

void Pipeline::writeDefaultPipelineConf(VkExtent2D extent) {

    PipelineConf *plconf = &(this->pipelineConfig);
    
    plconf->InputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    plconf->InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    plconf->InputAssemblyCI.primitiveRestartEnable = VK_FALSE;
    
    plconf->viewport.x = 0.0f;
    plconf->viewport.y = 0.0f;
    plconf->viewport.width = static_cast<float>(extent.width);
    plconf->viewport.height = static_cast<float>(extent.height);
    plconf->viewport.minDepth = 0.0f;
    plconf->viewport.maxDepth = 1.0f;
    plconf->scissor.offset = {0, 0};
    plconf->scissor.extent = extent;
    plconf->ViewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    plconf->ViewportCI.viewportCount = 1;
    plconf->ViewportCI.pViewports = &(plconf->viewport);
    plconf->ViewportCI.scissorCount = 1;
    plconf->ViewportCI.pScissors = &(plconf->scissor);

    plconf->RasterizationCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    plconf->RasterizationCI.depthClampEnable = VK_FALSE;
    plconf->RasterizationCI.rasterizerDiscardEnable = VK_FALSE;
    plconf->RasterizationCI.polygonMode = VK_POLYGON_MODE_FILL;
    plconf->RasterizationCI.lineWidth = 1.0f;
    plconf->RasterizationCI.cullMode = VK_CULL_MODE_NONE;
    plconf->RasterizationCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
    plconf->RasterizationCI.depthBiasEnable = VK_FALSE;
    plconf->RasterizationCI.depthBiasConstantFactor = 0.0f;  // Optional
    plconf->RasterizationCI.depthBiasClamp = 0.0f;           // Optional
    plconf->RasterizationCI.depthBiasSlopeFactor = 0.0f;     // Optional

    plconf->MultisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    plconf->MultisampleCI.sampleShadingEnable = VK_FALSE;
    plconf->MultisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    plconf->MultisampleCI.minSampleShading = 1.0f;           // Optional
    plconf->MultisampleCI.pSampleMask = nullptr;             // Optional
    plconf->MultisampleCI.alphaToCoverageEnable = VK_FALSE;  // Optional
    plconf->MultisampleCI.alphaToOneEnable = VK_FALSE;       // Optional

    plconf->colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    plconf->colorBlendAttachment.blendEnable = VK_FALSE;
    plconf->colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    plconf->colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    plconf->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    plconf->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    plconf->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    plconf->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

    plconf->ColorBlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    plconf->ColorBlendCI.logicOpEnable = VK_FALSE;
    plconf->ColorBlendCI.logicOp = VK_LOGIC_OP_COPY;  // Optional
    plconf->ColorBlendCI.attachmentCount = 1;
    plconf->ColorBlendCI.pAttachments = &(plconf->colorBlendAttachment);
    plconf->ColorBlendCI.blendConstants[0] = 0.0f;  // Optional
    plconf->ColorBlendCI.blendConstants[1] = 0.0f;  // Optional
    plconf->ColorBlendCI.blendConstants[2] = 0.0f;  // Optional
    plconf->ColorBlendCI.blendConstants[3] = 0.0f;  // Optional
    //
    plconf->DepthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    plconf->DepthStencilCI.depthTestEnable = VK_TRUE;
    plconf->DepthStencilCI.depthWriteEnable = VK_TRUE;
    plconf->DepthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
    plconf->DepthStencilCI.depthBoundsTestEnable = VK_FALSE;
    plconf->DepthStencilCI.minDepthBounds = 0.0f;  // Optional
    plconf->DepthStencilCI.maxDepthBounds = 1.0f;  // Optional
    plconf->DepthStencilCI.stencilTestEnable = VK_FALSE;
    plconf->DepthStencilCI.front = {};  // Optional
    plconf->DepthStencilCI.back = {};   // Optional
}
  VkDeviceSize offsets[] = {0};

void Pipeline::destroyPipeline() {
    vkDestroyPipeline(this->device->device, this->pipeline, nullptr);
}
void Pipeline::createPipeline(VkRenderPass renderPass) {

    std::array<VkPipelineShaderStageCreateInfo,2> shaderStages = {};

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = this->vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = this->fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    auto vertexBindingDescriptions = Model::getVertexBindingDescriptions();
    auto vertexAttributeDescriptions = Model::getVertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions    = vertexBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributeDescriptions.data();

    PipelineConf *plconf = &(this->pipelineConfig);
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    //pipelineInfo.pNext = nullptr;
    //pipelineInfo.flags = 0;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &(plconf->InputAssemblyCI);
    pipelineInfo.pViewportState      = &(plconf->ViewportCI);
    pipelineInfo.pRasterizationState = &(plconf->RasterizationCI);
    pipelineInfo.pMultisampleState   = &(plconf->MultisampleCI);
    pipelineInfo.pDepthStencilState  = &(plconf->DepthStencilCI);
    pipelineInfo.pColorBlendState    = &(plconf->ColorBlendCI);
    //pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = this->pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    //
    if (VK_SUCCESS != vkCreateGraphicsPipelines(
          this->device->device,
          VK_NULL_HANDLE,
          1,
          &pipelineInfo,
          nullptr,
          &(this->pipeline)
    )) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }
}


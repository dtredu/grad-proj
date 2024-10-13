
#include "types.hpp"
#include <vulkan/vulkan_core.h>

void Pipeline::writeDefaultPipelineConf(App *app) {

    PipelineConf *plconf = &(this->pipelineConfig);
    
    plconf->InputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    plconf->InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    plconf->InputAssemblyCI.primitiveRestartEnable = VK_FALSE;
    
    plconf->viewport.x = 0.0f;
    plconf->viewport.y = 0.0f;
    plconf->viewport.width = static_cast<float>(app->windowExtent.width);
    plconf->viewport.height = static_cast<float>(app->windowExtent.height);
    plconf->viewport.minDepth = 0.0f;
    plconf->viewport.maxDepth = 1.0f;
    plconf->scissor.offset = {0, 0};
    plconf->scissor.extent = {app->windowExtent.width, app->windowExtent.height};

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


//void buildPipelineCI(App* app) {
//  app->pipelineConfig.PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//  
//  app->pipelineConfig.PipelineCI.pInputAssemblyState = &(app->pipelineConfig.InputAssemblyCI);
//
//  app->pipelineConfig.ViewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//  app->pipelineConfig.ViewportCI.viewportCount = 1;
//  app->pipelineConfig.ViewportCI.pViewports = &(app->pipelineConfig.viewport);
//  app->pipelineConfig.ViewportCI.scissorCount = 1;
//  app->pipelineConfig.ViewportCI.pScissors = &(app->pipelineConfig.scissor);
//
//}


void createPipeline(App* app) {



}

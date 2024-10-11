
#include "types.hpp"


#include "main.vert.h"
#include "main.frag.h"

void createShaderModules(App *app) {
    app->pipelineConfig.vertShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    app->pipelineConfig.vertShaderCI.codeSize = sizeof(vertShaderCode);
    app->pipelineConfig.vertShaderCI.pCode = vertShaderCode;
    
    if (VK_SUCCESS != vkCreateShaderModule(
        app->device,
        &(app->pipelineConfig.vertShaderCI),
        nullptr,
        &(app->pipelineConfig.vertShader)
    )) {
        throw std::runtime_error("failed to create vertex shader module");
    }


    app->pipelineConfig.fragShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    app->pipelineConfig.fragShaderCI.codeSize = sizeof(fragShaderCode);
    app->pipelineConfig.fragShaderCI.pCode = fragShaderCode;
    
    if (VK_SUCCESS != vkCreateShaderModule(
        app->device,
        &(app->pipelineConfig.fragShaderCI),
        nullptr,
        &(app->pipelineConfig.fragShader)
    )) {
        throw std::runtime_error("failed to create fragment shader module");
    }
}
void destroyShaderModules(App *app) {
  vkDestroyShaderModule(app->device, app->pipelineConfig.vertShader, nullptr);
  vkDestroyShaderModule(app->device, app->pipelineConfig.fragShader, nullptr);
}

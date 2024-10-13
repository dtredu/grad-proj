
#include "types.hpp"

#include "main.vert.h" // present by CMake
#include "main.frag.h" // present by CMake

void Pipeline::createShaderModules(Device *device) {
    VkShaderModuleCreateInfo vertShaderCI;
    vertShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderCI.codeSize = sizeof(vertShaderCode);
    vertShaderCI.pCode = vertShaderCode;
    
    if (VK_SUCCESS != vkCreateShaderModule(
        device->device,
        &vertShaderCI,
        nullptr,
        &(this->shaders.vertShader)
    )) {
        throw std::runtime_error("failed to create vertex shader module");
    }


    VkShaderModuleCreateInfo fragShaderCI;
    fragShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderCI.codeSize = sizeof(fragShaderCode);
    fragShaderCI.pCode = fragShaderCode;
    
    if (VK_SUCCESS != vkCreateShaderModule(
        device->device,
        &fragShaderCI,
        nullptr,
        &(this->shaders.fragShader)
    )) {
        throw std::runtime_error("failed to create fragment shader module");
    }
}
void Pipeline::destroyShaderModules(Device *device) {
  vkDestroyShaderModule(device->device, this->shaders.vertShader, nullptr);
  vkDestroyShaderModule(device->device, this->shaders.fragShader, nullptr);
}

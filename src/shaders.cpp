
#include "types.hpp"

#include "main.vert.h" // present by CMake
#include "main.frag.h" // present by CMake

void Pipeline::createShaderModules() {
    VkShaderModuleCreateInfo vertShaderCI{};
    vertShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderCI.codeSize = sizeof(vertShaderCode);
    vertShaderCI.pCode = vertShaderCode;
    if(debug) {
        std::cout << "Vertex shader code size: " << vertShaderCI.codeSize << " bytes" << std::endl;
    }

    if (VK_SUCCESS != vkCreateShaderModule(
        this->device->device,
        &vertShaderCI,
        nullptr,
        &(this->vertShaderModule)
    )) {
        throw std::runtime_error("failed to create vertex shader module");
    }


    VkShaderModuleCreateInfo fragShaderCI{};
    fragShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderCI.codeSize = sizeof(fragShaderCode);
    fragShaderCI.pCode = fragShaderCode;
    if(debug) {
        std::cout << "Fragment shader code size: " << fragShaderCI.codeSize << " bytes" << std::endl;
    }

    if (VK_SUCCESS != vkCreateShaderModule(
        this->device->device,
        &fragShaderCI,
        nullptr,
        &(this->fragShaderModule)
    )) {
        throw std::runtime_error("failed to create fragment shader module");
    }
}

void Pipeline::destroyShaderModules() {
  vkDestroyShaderModule(this->device->device, this->vertShaderModule, nullptr);
  vkDestroyShaderModule(this->device->device, this->fragShaderModule, nullptr);
}

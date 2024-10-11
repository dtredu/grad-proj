
#pragma once
#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include "vulkan/vulkan.h"
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <string>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <SDL2/SDL_stdinc.h>
#include <cstdint>
#include <set>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily = {};
    std::optional<uint32_t> presentFamily = {};
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats = {};
    std::vector<VkPresentModeKHR> presentModes = {};
    bool isValid() {
        return !formats.empty() && !presentModes.empty();
    }
};

struct PipelineConf {
    VkGraphicsPipelineCreateInfo PipelineCI = {};
    
    // fixed function
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};

    VkPipelineViewportStateCreateInfo ViewportCI = {};
    VkViewport viewport = {};
    VkRect2D scissor {};
    
    VkPipelineRasterizationStateCreateInfo RasterizationCI = {};
    VkPipelineMultisampleStateCreateInfo MultisampleCI = {};
    VkPipelineDepthStencilStateCreateInfo DepthStencilCI = {};
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    VkPipelineColorBlendStateCreateInfo ColorBlendCI = {};

    // shaders
    VkShaderModuleCreateInfo vertShaderCI = {};
    VkShaderModuleCreateInfo fragShaderCI = {};
    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;
    std::vector <VkPipelineShaderStageCreateInfo> shaderStages = {};
};

struct App {
    bool debug = false;
    SDL_Window *window = nullptr;
    VkSurfaceKHR surface = nullptr;
    VkExtent2D windowExtent = {0,0}; // width, height

    VkApplicationInfo appInfo{};
    VkInstanceCreateInfo InstanceCI = {};
    std::vector<const char*> instanceExtensions = {};
    std::vector<const char*> instanceLayers = {};
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    std::vector<const char*> deviceExtensions = {};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
    VkDeviceCreateInfo DeviceCI = {};
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    SwapChainSupportDetails swapchainSupport = {};
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSwapchainCreateInfoKHR SwapChainCI = {};
        
    PipelineConf pipelineConfig = {};
};



bool compare(const char* a, const char* b) { return strcmp(a, b) < 0; };
bool isequal(const char* a, const char* b) { return strcmp(a, b) == 0; };

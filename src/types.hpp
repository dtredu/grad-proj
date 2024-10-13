
#pragma once

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include "vulkan/vulkan.h"
#include <SDL2/SDL_video.h>
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

inline bool debug = false;

inline bool compare(const char* a, const char* b) { return strcmp(a, b) < 0; };
inline bool isequal(const char* a, const char* b) { return strcmp(a, b) == 0; };

struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete() {
        return graphicsFamilyHasValue && presentFamilyHasValue;
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


struct App;

class Instance {
public:
    VkInstance instance = VK_NULL_HANDLE;
    std::vector<const char*> instanceExtensions = {};
    std::vector<const char*> instanceLayers = {};
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    void create(App *app);
    void destroy(App *app);

private:
    void parseExtensions (SDL_Window *window);
    void parseLayers();
    void setupDebugMessenger();
    void removeDebugMessenger();
    
};

class Device {
public:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    std::vector<const char*> deviceExtensions = {};

    SwapChainSupportDetails swapchainSupport = {};
    QueueFamilyIndices queueFamilies = {};

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findSupportedFormat(
        const std::vector<VkFormat> &candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    );

    void create(App *app);
    void destroy();
    void pickPhysicalDevice(App *app);
    void createImage(
        const VkImageCreateInfo &imageInfo,
        VkMemoryPropertyFlags properties,
        VkImage &image,
        VkDeviceMemory &depthImageMemory
    );
private:
    bool isPhysicalDeviceSuitble(App *app, VkPhysicalDevice phdev);
    bool areDeviceFeaturesSupported(VkPhysicalDevice phdev);
    bool areDeviceExtensionsSupported(VkPhysicalDevice phdev);
    QueueFamilyIndices findQueueFamilies(App *app, VkPhysicalDevice phdev);
    SwapChainSupportDetails querySwapChainSupport(App *app, VkPhysicalDevice phdev);
};

class SwapChain {
public:
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderpass = VK_NULL_HANDLE;

    uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    uint32_t imageCount = 0;
    std::vector<VkImage> swapChainImages = {};
    std::vector<VkImageView> swapChainImageViews = {};
    std::vector<VkImage> depthImages = {};
    std::vector<VkImageView> depthImageViews = {};
    std::vector<VkDeviceMemory> depthImageMemorys = {};
    std::vector<VkFramebuffer> swapChainFrameBuffers = {};

    VkFormat swapChainImageFormat = {};
    VkFormat swapChainDepthFormat = {};
    VkExtent2D swapChainExtent = {};

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    void createSwapChain(App *app);
    void createImageViews(Device *device);
    void createRenderPass(Device *device);
    void createDepthImagesViewsMemorys(Device *device);
    void createFrameBuffers(Device *device);

    void destroySwapChain(Device *device);
    void destroyImageViews(Device *device);
    void destroyRenderPass(Device *device);
    void destroyDepthImagesViewsMemorys(Device *device);
    void destroyFrameBuffers(Device *device);

    void createSemaphoresFences(Device *device);
    void destroySemaphoresFences(Device *device);


private:
    VkExtent2D chooseSwapExtent(App *app);
    VkPresentModeKHR chooseSwapPresentMode(App *app);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat (App *app);

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


struct Shaders {
    VkShaderModule vertShader = VK_NULL_HANDLE;
    VkShaderModule fragShader = VK_NULL_HANDLE;
};

class Pipeline {
public:
    Shaders shaders = {};
    PipelineConf pipelineConfig = {};
    void createShaderModules(Device *device);
    void destroyShaderModules(Device *device);

    void writeDefaultPipelineConf(App *app);
};

struct App {
    bool debug = false;
    SDL_Window *window = nullptr;
    VkSurfaceKHR surface = nullptr;
    VkExtent2D windowExtent = {0,0}; // width, height

    Instance instance;
    Device device;
    SwapChain swapchain;

    Pipeline pipeline;

};


struct Objects {
    VkSurfaceKHR surface = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages = {};
    std::vector <VkImageView> swapChainImageViews = {};


};


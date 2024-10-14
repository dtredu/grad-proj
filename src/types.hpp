
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
    void destroy();

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
    VkCommandPool commandPool = VK_NULL_HANDLE;

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
    void createCommandPool();
    void destroyCommandPool();
private:
    bool isPhysicalDeviceSuitble(App *app, VkPhysicalDevice phdev);
    bool areDeviceFeaturesSupported(VkPhysicalDevice phdev);
    bool areDeviceExtensionsSupported(VkPhysicalDevice phdev);
    QueueFamilyIndices findQueueFamilies(App *app, VkPhysicalDevice phdev);
    SwapChainSupportDetails querySwapChainSupport(App *app, VkPhysicalDevice phdev);
};

class SwapChain {
public:
    Device *device = nullptr;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderpass = VK_NULL_HANDLE;

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

    void createSwapChain(App *app);
    void createImageViews();
    void createRenderPass();
    void createDepthImagesViewsMemorys();
    void createFrameBuffers();

    void destroySwapChain();
    void destroyImageViews();
    void destroyRenderPass();
    void destroyDepthImagesViewsMemorys();
    void destroyFrameBuffers();


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


class Pipeline {
public:
    Device *device = nullptr;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkShaderModule vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragShaderModule = VK_NULL_HANDLE;
    
    PipelineConf pipelineConfig = {};

    void createShaderModules();
    void destroyShaderModules();

    void createPipelineLayout();
    void destroyPipelineLayout();

    void writeDefaultPipelineConf(VkExtent2D extent);
    void createPipeline(VkRenderPass renderPass);
    void destroyPipeline();
};

class Renderer {
public:
    Device *device = nullptr;
    SwapChain *swapchain = nullptr;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineBindPoint pipelineBindType;

    std::vector<VkCommandBuffer> commandBuffers = {};

    uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    size_t currentFrame = 0;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    
    void createSemaphoresFences();
    void destroySemaphoresFences();
    VkResult acquireNextImage(uint32_t *imageId);

    void createCommandBuffers();
    void destroyCommandBuffers();
    void recordCommandBuffers();
    VkResult submitCommandBuffers(const VkCommandBuffer *buffer, uint32_t *imageIndex);
    void drawFrame();
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

    Renderer renderer;
};


struct Objects {
    VkSurfaceKHR surface = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages = {};
    std::vector <VkImageView> swapChainImageViews = {};


};


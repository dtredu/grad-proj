#include "types.hpp"
#include <array>
#include <cstdint>
#include <vulkan/vulkan_core.h>



bool Device::areDeviceExtensionsSupported(VkPhysicalDevice phdev) {
    
    std::vector<const char*> requiredExtensions = this->deviceExtensions;
    
    std::vector<VkExtensionProperties> availableExtensions;
    uint32_t availableExtensionsCount;
    if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(phdev, nullptr, &availableExtensionsCount, nullptr)) {
        throw std::runtime_error("failed to enumerate phdev extensions");
    }
    availableExtensions.resize(availableExtensionsCount);
    if (VK_SUCCESS != vkEnumerateDeviceExtensionProperties(phdev, nullptr, &availableExtensionsCount, availableExtensions.data())) {
        throw std::runtime_error("failed to enumerate phdev extensions");
    }
    
    bool supported = false;
    for (const char* str : requiredExtensions) {
        supported=false;
        for (const VkExtensionProperties prop : availableExtensions) {
            if (isequal(str, prop.extensionName)) {
                supported = true;
                break;
            }
        }
        if (!supported) break;
    }
    return supported;
}

bool Device::areDeviceFeaturesSupported(VkPhysicalDevice phdev) {
    return true;
}

QueueFamilyIndices Device::findQueueFamilies(App *app, VkPhysicalDevice phdev) {
    QueueFamilyIndices indices{};
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &queueFamilyCount, queueFamilies.data());

    bool graphics;
    VkBool32 presentation;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceSupportKHR(phdev,i,app->surface,&presentation)) {
            throw std::runtime_error("couldn't query surface support for queue family");
        }
        graphics = ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0);
        
        if (graphics && (presentation == VK_TRUE)) {
            indices.graphicsFamily = i;
            indices.presentFamily = i;
            indices.graphicsFamilyHasValue = true;
            indices.presentFamilyHasValue = true;
            return indices;
        }
        if (graphics) {
            indices.graphicsFamily = i;
            indices.graphicsFamilyHasValue = true;
        }
        if (presentation == VK_TRUE) {
            indices.presentFamily = i;
            indices.presentFamilyHasValue = true;
        }
    }
    
    return indices;
}

SwapChainSupportDetails Device::querySwapChainSupport(App *app, VkPhysicalDevice phdev) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phdev, app->surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, app->surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, app->surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, app->surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        phdev,
        app->surface,
        &presentModeCount,
        details.presentModes.data());
  }
  return details;
}


bool Device::isPhysicalDeviceSuitble(App *app, VkPhysicalDevice phdev) {

    bool extensionsSupported = areDeviceExtensionsSupported(phdev);
    bool featuresSupported = areDeviceFeaturesSupported(phdev);

    bool queueFamiliesComplete = findQueueFamilies(app, phdev).isComplete();

    bool swapChainValid = false;
    if (extensionsSupported) {
        swapChainValid = querySwapChainSupport(app, phdev).isValid();
    }

    VkPhysicalDeviceProperties phdevProps;
    vkGetPhysicalDeviceProperties(phdev, &phdevProps);
    
    return extensionsSupported && featuresSupported && queueFamiliesComplete && swapChainValid;
}
//if (debug) {
//    std::cout << "  <" << phdevProps.deviceType << ">\"" << phdevProps.deviceName << "\"[" << availableExtensionsCount << "]" << std::endl;
//    //std::cout << std::endl;
//    //for (VkExtensionProperties extProps : availableExtensions) {
//    //    std::cout << extProps.extensionName << ' ';
//    //}
//    //std::cout << "\n\n" << std::endl;
//}


void Device::pickPhysicalDevice(App *app) {

    this->deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    std::vector <std::vector<VkPhysicalDevice>> phdevsByType(5,std::vector<VkPhysicalDevice> ());
    std::array <uint32_t,5> phdevTypeOrder = {1,2,3,4,0};

    std::vector<VkPhysicalDevice> phdevs;
    
    uint32_t phdevsCount;
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(app->instance.instance, &phdevsCount, nullptr)) {
        throw std::runtime_error("failed to enumerate the PhysicalDevices");
    }
    if (phdevsCount == 0) {
        throw std::runtime_error("no physical device found");
    }
    phdevs.resize(phdevsCount);
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(app->instance.instance, &phdevsCount, phdevs.data())) {
        throw std::runtime_error("failed to enumerate the PhysicalDevices");
    }
    if (debug) { std::cout << "physicalDevices[" << phdevsCount << "]:" << std::endl; }
    for (VkPhysicalDevice phdev : phdevs) {
        if (!isPhysicalDeviceSuitble(app, phdev)) continue;

        VkPhysicalDeviceProperties phdevProps;
        vkGetPhysicalDeviceProperties(phdev, &phdevProps);
        phdevsByType[phdevProps.deviceType].push_back(phdev);
    }
    
    for (int32_t t : phdevTypeOrder) {
        for (VkPhysicalDevice phdev : phdevsByType[t]) {
            physicalDevice = phdev;
            swapchainSupport = querySwapChainSupport(app, phdev);
            queueFamilies = findQueueFamilies(app, phdev);
            return;
        }
    }
    std::runtime_error("failed to find suitable physicalDevice");
}

void Device::destroy() {
    vkDestroyDevice(this->device, nullptr);
}
void Device::create(App *app) {

    QueueFamilyIndices indices = findQueueFamilies(app, this->physicalDevice);
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
    float queuePriority = 1.0f;

    //std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos = this->queueCreateInfos;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount    = queueCreateInfos.size();
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.enabledExtensionCount   = this->deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = this->deviceExtensions.data();
    
    if (VK_SUCCESS != vkCreateDevice(this->physicalDevice, &(createInfo), nullptr, &(this->device))) {
        std::runtime_error("failed to create vkDevice");
    }
    vkGetDeviceQueue(this->device, indices.graphicsFamily, 0, &(this->graphicsQueue));
    vkGetDeviceQueue(this->device, indices.presentFamily, 0, &(this->presentQueue));
    
    if (debug) {
        VkPhysicalDeviceProperties phdevProps;
        vkGetPhysicalDeviceProperties(this->physicalDevice, &phdevProps);
        std::cout << "picked \"" << phdevProps.deviceName << "\"" << std::endl;
        std::cout << "graphicsQueueIndex: " << indices.graphicsFamily << std::endl;
        std::cout << "presentQueueIndex:  " << indices.presentFamily << std::endl;
    }
}

// ############
//  INFO QUERY
// ############
//
VkFormat Device::findSupportedFormat(
    const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(this->physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (
            tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}


uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}


void Device::createImage(
    const VkImageCreateInfo &imageInfo,
    VkMemoryPropertyFlags properties,
    VkImage &image,
    VkDeviceMemory &imageMemory
) {
    if (vkCreateImage(this->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(this->device, image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (VK_SUCCESS != vkAllocateMemory(this->device, &allocInfo, nullptr, &imageMemory)) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    if (VK_SUCCESS != vkBindImageMemory(this->device, image, imageMemory, 0)) {
        throw std::runtime_error("failed to bind image memory!");
  }
}


// ################
//  RENDERER STUFF
// ################


void Device::destroyCommandPool() {
    vkDestroyCommandPool(this->device, this->commandPool, nullptr);
}
void Device::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = this->queueFamilies.graphicsFamily;
    poolInfo.flags =
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (VK_SUCCESS !=vkCreateCommandPool(this->device, &poolInfo, nullptr, &(this->commandPool))) {
        throw std::runtime_error("failed to create command pool!");
    }
}

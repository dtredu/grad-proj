#include "types.hpp"
#include <array>
#include <cstdint>
#include <vulkan/vulkan_core.h>



bool areDeviceExtensionsSupported(App *app, VkPhysicalDevice phdev) {
    
    std::vector<const char*> requiredExtensions = app->deviceExtensions;
    
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

bool areDeviceFeaturesSupported(App *app, VkPhysicalDevice phdev) {
    return true;
}

QueueFamilyIndices findQueueFamilies(App *app, VkPhysicalDevice phdev) {
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
            return indices;
        }
        if (graphics) indices.graphicsFamily = i;
        if (presentation == VK_TRUE) indices.presentFamily = i;
    }

    return indices;
}

SwapChainSupportDetails querySwapChainSupport(App *app, VkPhysicalDevice phdev) {
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


bool isPhysicalDeviceSuitble(App *app, VkPhysicalDevice phdev) {

    bool extensionsSupported = areDeviceExtensionsSupported(app, phdev);
    bool featuresSupported = areDeviceFeaturesSupported(app, phdev);

    bool queueFamiliesComplete = findQueueFamilies(app, phdev).isComplete();

    bool swapChainValid = false;
    if (extensionsSupported) {
        swapChainValid = querySwapChainSupport(app, phdev).isValid();
    }

    VkPhysicalDeviceProperties phdevProps;
    vkGetPhysicalDeviceProperties(phdev, &phdevProps);
    
    return extensionsSupported && featuresSupported && queueFamiliesComplete && swapChainValid;
}
//if (app->debug) {
//    std::cout << "  <" << phdevProps.deviceType << ">\"" << phdevProps.deviceName << "\"[" << availableExtensionsCount << "]" << std::endl;
//    //std::cout << std::endl;
//    //for (VkExtensionProperties extProps : availableExtensions) {
//    //    std::cout << extProps.extensionName << ' ';
//    //}
//    //std::cout << "\n\n" << std::endl;
//}


void pickPhysicalDevice(App *app) {
    
    std::vector <std::vector<VkPhysicalDevice>> phdevsByType(5,std::vector<VkPhysicalDevice> ());
    std::array <uint32_t,5> phdevTypeOrder = {1,2,3,4,0};

    std::vector<VkPhysicalDevice> phdevs;
    
    uint32_t phdevsCount;
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(app->instance, &phdevsCount, nullptr)) {
        throw std::runtime_error("failed to enumerate the PhysicalDevices");
    }
    if (phdevsCount == 0) {
        throw std::runtime_error("no physical device found");
    }
    phdevs.resize(phdevsCount);
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(app->instance, &phdevsCount, phdevs.data())) {
        throw std::runtime_error("failed to enumerate the PhysicalDevices");
    }
    if (app->debug) { std::cout << "physicalDevices[" << phdevsCount << "]:" << std::endl; }
    for (VkPhysicalDevice phdev : phdevs) {
        if (!isPhysicalDeviceSuitble(app, phdev)) continue;

        VkPhysicalDeviceProperties phdevProps;
        vkGetPhysicalDeviceProperties(phdev, &phdevProps);
        phdevsByType[phdevProps.deviceType].push_back(phdev);
    }
    
    for (int32_t t : phdevTypeOrder) {
        for (VkPhysicalDevice phdev : phdevsByType[t]) {
            app->physicalDevice = phdev;
            app->swapchainSupport = querySwapChainSupport(app, phdev);
            return;
        }
    }
    std::runtime_error("failed to find suitable physicalDevice");
}

void createDevice(App *app) {
    app->deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    pickPhysicalDevice(app);

    QueueFamilyIndices indices = findQueueFamilies(app, app->physicalDevice);
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        app->queueCreateInfos.push_back(queueCreateInfo);
    }
    
    app->DeviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    app->DeviceCI.queueCreateInfoCount = app->queueCreateInfos.size();
    app->DeviceCI.pQueueCreateInfos = app->queueCreateInfos.data();
    app->DeviceCI.enabledExtensionCount = app->deviceExtensions.size();
    app->DeviceCI.ppEnabledExtensionNames = app->deviceExtensions.data();
    
    if (VK_SUCCESS != vkCreateDevice(app->physicalDevice, &(app->DeviceCI), nullptr, &(app->device))) {
        std::runtime_error("failed to create vkDevice");
    }
    vkGetDeviceQueue(app->device, indices.graphicsFamily.value(), 0, &(app->graphicsQueue));
    vkGetDeviceQueue(app->device, indices.presentFamily.value() , 0, &(app->presentQueue));
    
    if (app->debug) {
        VkPhysicalDeviceProperties phdevProps;
        vkGetPhysicalDeviceProperties(app->physicalDevice, &phdevProps);
        std::cout << "picked \"" << phdevProps.deviceName << "\"" << std::endl;
        std::cout << "graphicsQueueIndex: " << indices.graphicsFamily.value() << std::endl;
        std::cout << "presentQueueIndex:  " << indices.presentFamily.value() << std::endl;
    }
}




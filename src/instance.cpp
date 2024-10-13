#include "types.hpp"
//#include "validation.cpp"
#include <SDL2/SDL_video.h>
#include <vulkan/vulkan_core.h>

// ###################
//  VALIDATION LAYERS
// ###################

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  std::cerr << "validation: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance,
      "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance,
        "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void Instance::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    if (CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &(this->debugMessenger)) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Instance::removeDebugMessenger() {
    DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
    this->debugMessenger = VK_NULL_HANDLE;
}


// ##########
//  INSTANCE
// ##########


void Instance::parseExtensions (SDL_Window *window) {
    std::vector<const char*> requiredExtensions = {
        //VK_KHR_SURFACE_EXTENSION_NAME,
    };
    if (debug){
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // get extensions required by SDL
    uint32_t requiredBySdlExtensionsCount;
    if (SDL_FALSE == SDL_Vulkan_GetInstanceExtensions(window, &requiredBySdlExtensionsCount, nullptr)) {
        throw std::runtime_error("failed to get instance extensions from SDL");
    }

    requiredExtensions.resize(requiredExtensions.size() + requiredBySdlExtensionsCount,nullptr);
    if (SDL_FALSE == SDL_Vulkan_GetInstanceExtensions(
        window,
        &requiredBySdlExtensionsCount,
        requiredExtensions.data() + (requiredExtensions.size() - requiredBySdlExtensionsCount)
    )) {
        throw std::runtime_error("failed to get instance extensions from SDL");
    }


    // dedupe requiredExtensions
    std::sort(requiredExtensions.begin(), requiredExtensions.end(),compare);
    auto last = std::unique(requiredExtensions.begin(), requiredExtensions.end(),isequal);
    requiredExtensions.erase(last, requiredExtensions.end());
    

    std::vector<VkExtensionProperties> availableExtensions;

    uint32_t availableExtensionsCount;
    if (VK_SUCCESS != vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr)) {
        throw std::runtime_error("failed to get available extensions from vulkan api");
    }
    availableExtensions.resize(availableExtensionsCount);
    if (VK_SUCCESS != vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.data())) {
        throw std::runtime_error("failed to get available extensions from vulkan api");
    }
    if (debug) {
        std::cout << "  availableExtensions[" << availableExtensions.size() << "]:" << std::endl;
        for (const VkExtensionProperties prop : availableExtensions) {
            std::cout << "    " << prop.extensionName << std::endl;
        }
        std::cout << "  requiredExtensions[" << requiredExtensions.size() << "]:" << std::endl;
        for (const char* str : requiredExtensions) {
            std::cout << "    " << str << std::endl;
        }
    }

    bool supported;
    for (const char* str : requiredExtensions) {
        supported=false;
        for (const VkExtensionProperties prop : availableExtensions) {
            if (isequal(str, prop.extensionName)) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            std::cerr << "Instance extension " << str << " is not supported!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    this->instanceExtensions = std::move(requiredExtensions);
}

void Instance::parseLayers() {

    std::vector<const char*> requiredLayers = {
    };

    if (debug){
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    }


    // dedupe requiredLayers
    std::sort(requiredLayers.begin(), requiredLayers.end(),compare);
    auto last = std::unique(requiredLayers.begin(), requiredLayers.end(),isequal);
    requiredLayers.erase(last, requiredLayers.end());

    std::vector<VkLayerProperties> availableLayers;

    uint32_t availableLayersCount;
    if (VK_SUCCESS != vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr)) {
        throw std::runtime_error("failed to get available layers from vulkan api");
    }
    availableLayers.resize(availableLayersCount);
    if (VK_SUCCESS != vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data())) {
        throw std::runtime_error("failed to get available layers from vulkan api");
    }

    if (debug) {
        std::cout << "  availableLayers[" << availableLayers.size() << "]:" << std::endl;
        for (const VkLayerProperties prop : availableLayers) {
            std::cout << "    " << prop.layerName << std::endl;
        }
        std::cout << "  requiredLayers[" << requiredLayers.size() << "]:" << std::endl;
        for (const char* str : requiredLayers) {
            std::cout << "    " << str << std::endl;
        }
    }

    bool supported;
    for (const char* str : requiredLayers) {
        supported=false;
        for (const VkLayerProperties prop : availableLayers) {
            if (isequal(str, prop.layerName)) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            std::cerr << "Instance layer " << str << " is not supported!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    this->instanceLayers = std::move(requiredLayers);
}

void Instance::create(App *app) {
  
    if (debug) {
      std::cout << "instanceCreation:" << std::endl;
    }
    parseExtensions(app->window);
    parseLayers();

    //VkApplicationInfo appInfo = this->appInfo;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    //VkInstanceCreateInfo createInfo = this->InstanceCI;
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount   = this->instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = this->instanceExtensions.data();
    createInfo.enabledLayerCount   = this->instanceLayers.size();
    createInfo.ppEnabledLayerNames = this->instanceLayers.data();

    if (vkCreateInstance(&createInfo, nullptr, &(this->instance)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create the instance");
    }
    if (debug) {
        setupDebugMessenger();
    }
}

void Instance::destroy(App *app) {
    if (debug) {
        removeDebugMessenger();
    }
    vkDestroyInstance(this->instance, nullptr);
}


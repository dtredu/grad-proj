#include "types.hpp"
#include "validation.cpp"
#include <vulkan/vulkan_core.h>


void parseInstanceExtensions (App *app) {
    std::vector<const char*> requiredExtensions = {
        //VK_KHR_SURFACE_EXTENSION_NAME,
    };
    if (app->debug){
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // get extensions required by SDL
    uint32_t requiredBySdlExtensionsCount;
    if (SDL_FALSE == SDL_Vulkan_GetInstanceExtensions(app->window, &requiredBySdlExtensionsCount, nullptr)) {
        throw std::runtime_error("failed to get instance extensions from SDL");
    }

    requiredExtensions.resize(requiredExtensions.size() + requiredBySdlExtensionsCount,nullptr);
    if (SDL_FALSE == SDL_Vulkan_GetInstanceExtensions(
        app->window,
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
    if (app->debug) {
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
    app->instanceExtensions = std::move(requiredExtensions);
}

void parseInstanceLayers(App *app) {

    std::vector<const char*> requiredLayers = {
    };

    if (app->debug){
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

    if (app->debug) {
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

    app->instanceLayers = std::move(requiredLayers);
}

void createInstance(App *app) {
  
    if (app->debug) {
      std::cout << "instanceCreation:" << std::endl;
    }
    parseInstanceExtensions(app);
    parseInstanceLayers(app);

    app->appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app->appInfo.pApplicationName = "Hello Triangle";
    app->appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app->appInfo.pEngineName = "No Engine";
    app->appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app->appInfo.apiVersion = VK_API_VERSION_1_0;

    app->InstanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    app->InstanceCI.pApplicationInfo = &(app->appInfo);
    app->InstanceCI.enabledExtensionCount = app->instanceExtensions.size();
    app->InstanceCI.ppEnabledExtensionNames = app->instanceExtensions.data();
    app->InstanceCI.enabledLayerCount = app->instanceLayers.size();
    app->InstanceCI.ppEnabledLayerNames = app->instanceLayers.data();

    if (vkCreateInstance(&(app->InstanceCI), nullptr, &(app->instance)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create the instance");
    }
    if (app->debug) {
        setupDebugMessenger(app);
    }
}

void destroyInstance(App *app) {
    if (app->debug) {
        removeDebugMessenger(app);
    }
    vkDestroyInstance(app->instance, nullptr);
}


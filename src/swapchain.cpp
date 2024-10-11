
#include "types.hpp"
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

VkSurfaceFormatKHR chooseSwapSurfaceFormat (App *app) {
    for (const auto &availableFormat : app->swapchainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return app->swapchainSupport.formats[0];
}

VkPresentModeKHR chooseSwapPresentMode(App *app) {
    for (const auto &availablePresentMode : app->swapchainSupport.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            if (app->debug) std::cout << "Present mode: Mailbox" << std::endl;
            return availablePresentMode;
        }
    }

    //for (const auto &availablePresentMode : app->swapchainSupport.presentModes) {
    //    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //        if (app->debug) std::cout << "Present mode: Immediate" << std::endl;
    //        return availablePresentMode;
    //    }
    //}

    if (app->debug) std::cout << "Present mode: V-Sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(App *app) {
    const VkSurfaceCapabilitiesKHR &capabilities = app->swapchainSupport.capabilities;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = app->windowExtent;
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

void createSwapChain(App *app) {

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(app);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(app);
    VkExtent2D extent = chooseSwapExtent(app);

    uint32_t imageCount = app->swapchainSupport.capabilities.minImageCount + 1;
    if (app->swapchainSupport.capabilities.maxImageCount > 0 &&
        imageCount > app->swapchainSupport.capabilities.maxImageCount) {
        imageCount = app->swapchainSupport.capabilities.maxImageCount;
    }

    app->SwapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    app->SwapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    app->SwapChainCI.surface = app->surface;

    app->SwapChainCI.minImageCount = imageCount;
    app->SwapChainCI.imageFormat = surfaceFormat.format;
    app->SwapChainCI.imageColorSpace = surfaceFormat.colorSpace;
    app->SwapChainCI.imageExtent = extent;
    app->SwapChainCI.imageArrayLayers = 1;
    app->SwapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

}



void createRenderPass(App *app) {






}




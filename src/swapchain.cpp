
#include "types.hpp"
#include <vulkan/vulkan_core.h>

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat (App *app) {
    for (const auto &availableFormat : app->device.swapchainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return app->device.swapchainSupport.formats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(App *app) {
    for (const auto &availablePresentMode : app->device.swapchainSupport.presentModes) {
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

VkExtent2D SwapChain::chooseSwapExtent(App *app) {
    const VkSurfaceCapabilitiesKHR &capabilities = app->device.swapchainSupport.capabilities;
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

void SwapChain::destroySwapChain() {
    vkDestroySwapchainKHR(this->device->device, this->swapchain, nullptr);
}
void SwapChain::createSwapChain(App *app) {

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(app);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(app);
    VkExtent2D extent = chooseSwapExtent(app);

    this->imageCount = app->device.swapchainSupport.capabilities.minImageCount + 1;
    if (app->device.swapchainSupport.capabilities.maxImageCount > 0 &&
        imageCount > app->device.swapchainSupport.capabilities.maxImageCount) {
        imageCount = app->device.swapchainSupport.capabilities.maxImageCount;
    }

    //VkSwapchainCreateInfoKHR &createInfo = this->SwapChainCI;
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = app->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if (app->device.queueFamilies.graphicsFamily != app->device.queueFamilies.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = reinterpret_cast<uint32_t*>(
           app->device.queueFamilies.graphicsFamily // + presentFamily
        );
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = app->device.swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = nullptr;
    //this->SwapChainCI.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

    if (vkCreateSwapchainKHR(app->device.device, &(createInfo), nullptr, &(this->swapchain)) != VK_SUCCESS) {
      throw std::runtime_error("failed to create swap chain");
    }

    // we only specified a minimum number of images in the swap chain, so the implementation is
    // allowed to create a swap chain with more. That's why we'll first query the final number of
    // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
    // retrieve the handles.

    vkGetSwapchainImagesKHR(app->device.device, this->swapchain, &(this->imageCount), nullptr);
    this->swapChainImages.resize(this->imageCount);
    vkGetSwapchainImagesKHR(app->device.device, this->swapchain, &(this->imageCount), this->swapChainImages.data());

    this->swapChainImageFormat = surfaceFormat.format;
    this->swapChainExtent = extent;
}


void SwapChain::destroyImageViews() {
    for (auto imageView : this->swapChainImageViews) {
        vkDestroyImageView(this->device->device, imageView, nullptr);
    }
}
void SwapChain::createImageViews() {
    this->swapChainImageViews.resize(this->imageCount);
    for (size_t i = 0; i < this->imageCount; i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = this->swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = this->swapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (VK_SUCCESS != vkCreateImageView(this->device->device, &viewInfo, nullptr, &(this->swapChainImageViews[i]))) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }
}


void SwapChain::destroyRenderPass() {
    vkDestroyRenderPass  (this->device->device, this->renderpass, nullptr);
}
void SwapChain::createRenderPass() {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = device->findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = this->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    std::array<VkAttachmentDescription,2> renderPassAttachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassInfo.pAttachments = renderPassAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (VK_SUCCESS != vkCreateRenderPass(this->device->device, &renderPassInfo, nullptr, &(this->renderpass))) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void SwapChain::destroyDepthImagesViewsMemorys() {
    for (int i = 0; i < this->depthImages.size(); i++) {
        vkDestroyImageView(this->device->device, this->depthImageViews[i], nullptr);
        vkDestroyImage(this->device->device, this->depthImages[i], nullptr);
        vkFreeMemory(this->device->device, this->depthImageMemorys[i], nullptr);
    }
}
void SwapChain::createDepthImagesViewsMemorys() {
    
    this->swapChainDepthFormat = device->findSupportedFormat(
          {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
          VK_IMAGE_TILING_OPTIMAL,
          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    VkExtent2D swapChainExtent = this->swapChainExtent;
    
    this->depthImages.resize(this->imageCount);
    this->depthImageMemorys.resize(this->imageCount);
    this->depthImageViews.resize(this->imageCount);

    for (int i = 0; i < depthImages.size(); i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = this->swapChainDepthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        device->createImage(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImages[i],
            depthImageMemorys[i]
        );

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = this->swapChainDepthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (VK_SUCCESS != vkCreateImageView(this->device->device, &viewInfo, nullptr, &(this->depthImageViews[i]))) {
          throw std::runtime_error("failed to create texture image view!");
        }
    }
}

void SwapChain::destroyFrameBuffers() {
    for (auto framebuffer : this->swapChainFrameBuffers) {
        vkDestroyFramebuffer(this->device->device, framebuffer, nullptr);
    }
};
void SwapChain::createFrameBuffers() {
    swapChainFrameBuffers.resize(this->imageCount);
    for (size_t i = 0; i < this->imageCount; i++) {
        std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = this->renderpass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = this->swapChainExtent.width;
        framebufferInfo.height = this->swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (VK_SUCCESS != vkCreateFramebuffer(
            this->device->device,
            &framebufferInfo,
            nullptr,
            &(this->swapChainFrameBuffers[i])
        )) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}


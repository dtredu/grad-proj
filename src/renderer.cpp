#include "types.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

// ############
//  SYNC STUFF
// ############

void Renderer::destroySemaphoresFences() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(this->device->device, this->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(this->device->device, this->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(this->device->device, this->inFlightFences[i], nullptr);
    }
}
void Renderer::createSemaphoresFences() {
  this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  this->imagesInFlight.resize(this->swapchain->imageCount, VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (VK_SUCCESS != vkCreateSemaphore(this->device->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) ||
        VK_SUCCESS != vkCreateSemaphore(this->device->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) ||
        VK_SUCCESS != vkCreateFence(this->device->device, &fenceInfo, nullptr, &this->inFlightFences[i])
    ) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }
}

VkResult Renderer::acquireNextImage(uint32_t *imageId) {
    vkWaitForFences(
        this->device->device,
        1,
        &this->inFlightFences[this->currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max()
    );
    VkResult result = vkAcquireNextImageKHR(
        this->device->device,
        this->swapchain->swapchain,
        std::numeric_limits<uint64_t>::max(),
        this->imageAvailableSemaphores[this->currentFrame],  // must be a not signaled semaphore
        VK_NULL_HANDLE,
        imageId
    );
    return result;
}

// #################
//  COMMAND BUFFERS
// #################

void Renderer::createCommandBuffers() {
    this->commandBuffers.resize(this->swapchain->imageCount);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = this->device->commandPool;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.size());

    if (VK_SUCCESS != vkAllocateCommandBuffers(this->device->device, &allocateInfo, this->commandBuffers.data())) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}
void Renderer::destroyCommandBuffers() {
    vkFreeCommandBuffers(
        this->device->device,
        this->device->commandPool,
        static_cast<uint32_t>(this->commandBuffers.size()),
        this->commandBuffers.data());
    this->commandBuffers.clear();
}

void Renderer::recordCommandBuffers() {
    for (size_t i = 0; i < this->commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                   // Optional
        beginInfo.pInheritanceInfo = nullptr;  // Optional

        if (VK_SUCCESS != vkBeginCommandBuffer(this->commandBuffers[i], &beginInfo)) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = this->swapchain->renderpass;
        renderPassInfo.framebuffer = this->swapchain->swapChainFrameBuffers[i];

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = this->swapchain->swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(this->commandBuffers[i], this->pipelineBindType, this->pipeline);
        
        vkCmdDraw(this->commandBuffers[i], 3, 1, 0, 0);
        //lveModel.bind(commandBuffers[i]);
        //lveModel.draw(commandBuffers[i]);

        vkCmdEndRenderPass(this->commandBuffers[i]);
        if (VK_SUCCESS != vkEndCommandBuffer(this->commandBuffers[i])) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

VkResult Renderer::submitCommandBuffers(const VkCommandBuffer *buffer, uint32_t *imageId) {
    uint32_t &MAX_FRAMES_IN_FLIGHT = this->MAX_FRAMES_IN_FLIGHT;

    VkFence &imageInFlight = this->imagesInFlight[*imageId];

    if (VK_NULL_HANDLE != imageInFlight) {
        vkWaitForFences(this->device->device, 1, &imageInFlight, VK_TRUE, UINT64_MAX);
    }
    imageInFlight = this->inFlightFences[this->currentFrame];

    VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->currentFrame]};
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(this->device->device, 1, &this->inFlightFences[this->currentFrame]);
    if (VK_SUCCESS != vkQueueSubmit(this->device->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame])) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {this->swapchain->swapchain};
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageId;

    auto result = vkQueuePresentKHR(this->device->presentQueue, &presentInfo);

    this->currentFrame = (this->currentFrame + 1) % this->MAX_FRAMES_IN_FLIGHT;

    return result;
}

void Renderer::drawFrame() {

    uint32_t imageId;
    auto result = this->acquireNextImage(&imageId);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    result = this->submitCommandBuffers(&this->commandBuffers[imageId], &imageId);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "present_result = " << result << std::endl;
        throw std::runtime_error("failed to present swap chain image!");
    }
}

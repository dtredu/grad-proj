#include "types.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

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
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                   // Optional
        beginInfo.pInheritanceInfo = nullptr;  // Optional

        if (VK_SUCCESS != vkBeginCommandBuffer(commandBuffers[i], &beginInfo)) {
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

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], this->pipelineBindType, this->pipeline);
        
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
        //lveModel.bind(commandBuffers[i]);
        //lveModel.draw(commandBuffers[i]);

        vkCmdEndRenderPass(commandBuffers[i]);
        if (VK_SUCCESS != vkEndCommandBuffer(commandBuffers[i])) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

VkResult Renderer::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageId) {
    std::vector<VkFence> &imagesInFlight = this->swapchain->imagesInFlight;
    std::vector<VkFence> &inFlightFences = this->swapchain->inFlightFences;
    std::vector<VkSemaphore> &imageAvailableSemaphores = this->swapchain->imageAvailableSemaphores;
    std::vector<VkSemaphore> &renderFinishedSemaphores = this->swapchain->renderFinishedSemaphores;
    size_t &currentFrame = this->swapchain->currentFrame;
    uint32_t &MAX_FRAMES_IN_FLIGHT = this->swapchain->MAX_FRAMES_IN_FLIGHT;
    VkFence &imageInFlight = imagesInFlight[*imageId];
    

    if (VK_NULL_HANDLE != imageInFlight) {
        vkWaitForFences(this->device->device, 1, &imageInFlight, VK_TRUE, UINT64_MAX);
    }
    imageInFlight = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(this->device->device, 1, &inFlightFences[currentFrame]);
    if (VK_SUCCESS != vkQueueSubmit(this->device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame])) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {this->swapchain->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageId;

    auto result = vkQueuePresentKHR(this->device->presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}


void Renderer::drawFrame() {

    uint32_t imageId;
    auto result = this->swapchain->acquireNextImage(&imageId);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    result = this->submitCommandBuffers(&commandBuffers[imageId], &imageId);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "present_result = " << result << std::endl;
        throw std::runtime_error("failed to present swap chain image!");
    }
}

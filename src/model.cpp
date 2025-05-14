
#include "types.hpp"
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <vulkan/vulkan_core.h>

int Model::loadImageSTBI() {

    this->stb_image.pixels = stbi_load(
        (this->stb_image.path.c_str()), 
        &(this->stb_image.texWidth),
        &(this->stb_image.texHeight),
        &(this->stb_image.texChannels),
        STBI_rgb_alpha
    );
    if (nullptr == this->stb_image.pixels) {
        return 1;
    }

    this->stb_image.size = this->stb_image.texWidth * this->stb_image.texHeight * 4;

    return 0;
}

void Model::createTextureObjects() {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = this->stb_image.texWidth;
    imageInfo.extent.height = this->stb_image.texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;

    this->device->createImage(
        imageInfo,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        this->textureImage,
        this->textureMemory
    );

    this->device->createBuffer(
        this->stb_image.size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        this->textureStagingBuffer,
        this->textureStagingMemory
    );

    vkMapMemory(
        this->device->device,
        this->textureStagingMemory,
        0,
        this->stb_image.size,
        0, //VkMemoryMapFlags flags,
        &(this->textureStagingData) // void **ppData
    );


    //VkImageCreateInfo stagingImageInfo{};
    //imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    //imageInfo.imageType = VK_IMAGE_TYPE_2D;
    //imageInfo.extent.width = this->stb_image.texWidth;
    //imageInfo.extent.height = this->stb_image.texHeight;
    //imageInfo.extent.depth = 1;
    //imageInfo.mipLevels = 1;
    //imageInfo.arrayLayers = 1;
    //imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    //imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    //imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    //imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    //imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //imageInfo.flags = 0;

    //VkDeviceSize stagingSize;
    //this->device->createImage(
    //    stagingImageInfo,
    //    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //    this->textureStagingImage,
    //    this->textureStagingMemory,
    //    this->textureStagingSize
    //);
    
    //if (this->textureStagingSize != this->stb_image.size) {
    //    std::cerr << "Image sizes are different!" << std::endl;
    //}
}

void Model::destroyTextureObjects() {
    vkUnmapMemory(this->device->device, this->textureStagingMemory);

    vkDestroyBuffer(this->device->device, this->textureStagingBuffer, nullptr);
    vkDestroyImage(this->device->device, this->textureImage, nullptr);

    vkFreeMemory(this->device->device, this->textureStagingMemory, nullptr);
    vkFreeMemory(this->device->device, this->textureMemory, nullptr);
}

//void transitionImageLayout(VkCommandBuffer &commandBuffer,VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
//    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
//
//    endSingleTimeCommands(commandBuffer);
//}

void Model::writeTextureToGPU() {

    //this->commandBuffers.resize(this->swapchain->imageCount);
    memcpy(this->textureStagingData, this->stb_image.pixels, this->textureStagingSize);

    // create cmd buffer
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = this->device->commandPool;
    allocateInfo.commandBufferCount = 1; //static_cast<uint32_t>(this->commandBuffers.size());

    if (VK_SUCCESS != vkAllocateCommandBuffers(this->device->device, &allocateInfo, &commandBuffer)) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // begin cmd buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    if (VK_SUCCESS != vkBeginCommandBuffer(commandBuffer, &beginInfo)) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // image layout: UNDEFINED -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier1{};
    barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier1.srcAccessMask = 0;
    barrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier1.image = this->textureImage;
    barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier1.subresourceRange.baseMipLevel = 0;
    barrier1.subresourceRange.levelCount = 1;
    barrier1.subresourceRange.baseArrayLayer = 0;
    barrier1.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier1
    );

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;

    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent.width = this->stb_image.texWidth;
    copyRegion.imageExtent.height = this->stb_image.texHeight;
    copyRegion.imageExtent.depth = 1;
    
    vkCmdCopyBufferToImage(
        commandBuffer,
        this->textureStagingBuffer,
        this->textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );

    // image layout:  TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier barrier2{};
    barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier2.image = this->textureImage;
    barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier2.subresourceRange.baseMipLevel = 0;
    barrier2.subresourceRange.levelCount = 1;
    barrier2.subresourceRange.baseArrayLayer = 0;
    barrier2.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier2
    );
    //vkCmdCopyImage(
    //    commandBuffer,
    //    this->textureStagingImage, VkImageLayout srcImageLayout,
    //    this->textureImage,        VkImageLayout dstImageLayout,
    //    1, &copyRegion
    //);
    //vkCmdCopyImage(commandBuffer, this->textureStagingImage, textureImage, 1, &copyRegion);
    //VkRenderPassBeginInfo renderPassInfo = {};
    //renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //renderPassInfo.renderPass = this->swapchain->renderpass;
    //renderPassInfo.framebuffer = this->swapchain->swapChainFrameBuffers[i];

    //renderPassInfo.renderArea.offset = {0, 0};
    //renderPassInfo.renderArea.extent = this->swapchain->swapChainExtent;

    //std::array<VkClearValue, 2> clearValues{};
    //clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
    //clearValues[1].depthStencil = {1.0f, 0};
    //renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    //renderPassInfo.pClearValues = clearValues.data();

    //vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    //vkCmdBindPipeline(this->commandBuffers[i], this->pipelineBindType, this->pipeline);
      
    //vkCmdDraw(this->commandBuffers[i], 3, 1, 0, 0);
    //model->bind(commandBuffers[i]);
    //model->draw(commandBuffers[i]);

    if (VK_SUCCESS != vkEndCommandBuffer(commandBuffer)) {
        throw std::runtime_error("failed to record command buffer!");
    }


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (VK_SUCCESS != vkQueueSubmit(this->device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE)) {
        throw std::runtime_error("failed to submit copy command buffer!");
    }
    vkQueueWaitIdle(this->device->graphicsQueue);

    vkFreeCommandBuffers(this->device->device, this->device->commandPool, 1, &commandBuffer);
}


void Model::destroyVertexBuffers() {
    vkDestroyBuffer(this->device->device, vertexBuffer, nullptr);
    vkFreeMemory(this->device->device, vertexBufferMemory, nullptr);
}

void Model::createVertexBuffers(size_t maxVertexCount) {
    this->maxVertexCount = maxVertexCount;
    assert(this->maxVertexCount >= 3 && "Vertex count must be at least 3");
    VkDeviceSize bufferSize = sizeof(Vertex) * maxVertexCount;
    this->device->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        this->vertexBuffer,
        this->vertexBufferMemory
    );
    //this->writeVertexBuffers(this->vertices);
}
void Model::writeVertexBuffers(const std::vector<Vertex> &vertices) {
    VkDeviceSize bufferSize = sizeof(Vertex) * std::min(maxVertexCount,vertices.size());
    assert(bufferSize > 0 && "number of vertices must be at least > 0");
    void *data;
    vkMapMemory(this->device->device, this->vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(this->device->device, this->vertexBufferMemory);
}


void Model::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, this->vertexCount, 1, 0, 0);
}


void Model::bind(VkCommandBuffer commandBuffer) {
  VkBuffer buffers[] = {this->vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

std::vector<VkVertexInputBindingDescription> Model::getVertexBindingDescriptions() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(Vertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Model::getVertexAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = 0;

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, texCoord);
  return attributeDescriptions;
}


#include "types.hpp"
#include <cstdint>
#include <algorithm>

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
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = 0;
  return attributeDescriptions;
}

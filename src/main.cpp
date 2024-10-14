#include "types.hpp"
#include <SDL2/SDL_video.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

void run_app(App *app) {
    // window -> Instance -> Surface -> Device -> Swapchain
    SDL_Init(SDL_INIT_VIDEO);
    app->window = SDL_CreateWindow(
        "hello-triangle",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1280,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
    );
    app->instance.create(app);
    if (SDL_TRUE != SDL_Vulkan_CreateSurface(app->window,app->instance.instance,&(app->surface))) {
        throw std::runtime_error("failed to create the surface");
    }
    app->device.pickPhysicalDevice(app);
    app->device.create(app);
    
    app->swapchain.device = &(app->device);
    app->swapchain.createSwapChain(app);
    app->swapchain.createImageViews();
    app->swapchain.createRenderPass();
    app->swapchain.createDepthImagesViewsMemorys();
    app->swapchain.createFrameBuffers();


    app->pipeline.device = &(app->device);
    app->pipeline.createShaderModules();
    app->pipeline.createPipelineLayout();
    app->pipeline.writeDefaultPipelineConf(app->swapchain.swapChainExtent);
    app->pipeline.createPipeline(app->swapchain.renderpass);


    app->renderer.device = &(app->device);
    app->renderer.swapchain = &(app->swapchain);
    app->renderer.pipeline = app->pipeline.pipeline;
    app->renderer.pipelineBindType = VK_PIPELINE_BIND_POINT_GRAPHICS;
    app->renderer.createSemaphoresFences();
    app->device.createCommandPool();
    app->renderer.createCommandBuffers();
    app->renderer.recordCommandBuffers();

    bool running = true;
    while(running) {
        SDL_Event windowEvent;
        while(SDL_PollEvent(&windowEvent))
            if(windowEvent.type == SDL_QUIT) {
                running = false;
                break;
            }
        app->renderer.drawFrame();
    }

    //if (!SDL_Vulkan_DestroySurface(app->window,app->surface)) {
    //    throw std::runtime_error("failed to destroy the surface");
    //}
    
    vkDeviceWaitIdle(app->device.device);

    app->renderer.destroyCommandBuffers();
    app->device.destroyCommandPool();
    app->renderer.destroySemaphoresFences();
    app->renderer.swapchain = nullptr;
    app->renderer.device = nullptr;

    app->pipeline.destroyPipeline();
    app->pipeline.destroyPipelineLayout();
    app->pipeline.destroyShaderModules ();
    app->pipeline.device = nullptr;

    app->swapchain.destroyFrameBuffers();
    app->swapchain.destroyDepthImagesViewsMemorys();
    app->swapchain.destroyRenderPass();
    app->swapchain.destroyImageViews();
    app->swapchain.destroySwapChain();
    app->swapchain.device = nullptr;
    
    app->device.destroy();
    vkDestroySurfaceKHR(app->instance.instance, app->surface, nullptr);
    app->instance.destroy();
    //vkDestroyInstance(vkInst, nullptr);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
    
    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    return;
}

int main() {
    App app{};
    debug = true;
    run_app(&app);
    return 0;
}

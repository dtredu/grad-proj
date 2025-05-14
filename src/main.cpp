
#define STB_IMAGE_IMPLEMENTATION
#include "types.hpp"
#include <SDL2/SDL_video.h>
#include <vulkan/vulkan_core.h>

void run_app(App *app) {
    // window -> Instance -> Surface -> Device -> Swapchain ->
    // -> Pipeline -> Vertex Buffers -> Renderer
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
    app->device.createCommandPool();
    
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
    app->pipeline.pipelineConfig.InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; // LIST | STRIP
    app->pipeline.pipelineConfig.RasterizationCI.cullMode = VK_CULL_MODE_BACK_BIT;
    app->pipeline.createPipeline(app->swapchain.renderpass);

    app->model.device = &(app->device);
    //app->model.vertices = {{{0.0f, -0.5f}}, {{0.5f, 0.5f}}, {{-0.5f, 0.5f}}};
    app->model.createTextureObjects();
    app->model.createVertexBuffers(10);
    app->model.vertices = {
        {{ -0.5,  0.75}},
        {{  0.0, -0.75}},
        {{  0.5,  0.75}},

        //{{  0.0, -0.75}}, // culled without strip
        //{{  0.5,  0.75}},
        {{  0.75,-0.75}},


        // if using STRIP, then triangles do not have alternating faces
        // https://stackoverflow.com/questions/9154117/back-face-culling-gl-triangle-strip
    };
    //app->model.vertexCount = 3;
    app->model.vertexCount = app->model.vertices.size();
    app->model.writeVertexBuffers(app->model.vertices);
    app->model.writeTextureToGPU();

    app->renderer.device = &(app->device);
    app->renderer.swapchain = &(app->swapchain);
    app->renderer.pipeline = app->pipeline.pipeline;
    app->renderer.pipelineBindType = VK_PIPELINE_BIND_POINT_GRAPHICS;
    app->renderer.createSemaphoresFences();
    app->renderer.createCommandBuffers();
    app->renderer.recordCommandBuffers(&app->model);

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
    app->renderer.destroySemaphoresFences();
    app->renderer.swapchain = nullptr;
    app->renderer.device = nullptr;

    app->model.destroyVertexBuffers();
    app->model.destroyTextureObjects();

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
    
    app->device.destroyCommandPool();
    app->device.destroy();
    vkDestroySurfaceKHR(app->instance.instance, app->surface, nullptr);
    app->instance.destroy();
    //vkDestroyInstance(vkInst, nullptr);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
    return;
}

int main(int argc, char* argv[]) {
    App app{};
    debug = true;

    if (argc > 1) {
        app.model.stb_image.path = argv[1];
    } else {
        std::cerr << "No image path provided!" << std::endl;
        return 1;
    }
    int retcode = app.model.loadImageSTBI();
    if (0 != retcode) {
        std::cerr << "Failed to load the image!" << std::endl;
        return 1;
    }


    run_app(&app);
    return 0;
}

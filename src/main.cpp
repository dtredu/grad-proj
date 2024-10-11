#include "types.hpp"
#include "instance.cpp"
#include "device.cpp"
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
        640,
        360,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN
    );
    createInstance(app);
    if (SDL_TRUE != SDL_Vulkan_CreateSurface(app->window,app->instance,&(app->surface))) {
        throw std::runtime_error("failed to create the surface");
    }
    createDevice(app);
  

    bool running = true;
    while(running) {
        SDL_Event windowEvent;
        while(SDL_PollEvent(&windowEvent))
            if(windowEvent.type == SDL_QUIT) {
                running = false;
                break;
            }
    }


    //vkDestroyDevice(device, nullptr);
    //if (!SDL_Vulkan_DestroySurface(app->window,app->surface)) {
    //    throw std::runtime_error("failed to destroy the surface");
    //}
    vkDestroyDevice(app->device, nullptr);
    vkDestroySurfaceKHR(app->instance, app->surface, nullptr);
    destroyInstance(app);
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
    app.debug = true;
    run_app(&app);
    return 0;
}

cmake_minimum_required(VERSION 3.10)

project(main VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)
#find_package(glfw3 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

find_package(Vulkan COMPONENTS glslc REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
message(${glslc_executable})


include_directories(${Vulkan_INCLUDE_DIRS})
#include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})

# shaders
add_custom_command(
    OUTPUT src/main.vert.h
    DEPENDS shaders/main.vert.glsl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND echo -n "const uint32_t vertShaderCode[] = " > src/main.vert.h
    COMMAND ${glslc_executable} -mfmt=c -fshader-stage=vert shaders/main.vert.glsl -o - >> src/main.vert.h
    COMMAND echo "\;" >> src/main.vert.h
)

add_custom_command(
    OUTPUT src/main.frag.h
    DEPENDS shaders/main.frag.glsl
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND echo -n "const uint32_t fragShaderCode[] = " > src/main.frag.h
    COMMAND ${glslc_executable} -mfmt=c -fshader-stage=frag shaders/main.frag.glsl -o - >> src/main.frag.h
    COMMAND echo "\;" >> src/main.frag.h
)


# compile the main executable
file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(main ${SRC_FILES})
target_sources(main PRIVATE src/main.vert.h src/main.frag.h)


target_link_libraries(main ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES} glm::glm) # glfw

set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)



#add_executable(main main.cpp)

# Optional: Add compiler flags for better debugging
#if (CMAKE_BUILD_TYPE MATCHES Debug)
#    target_compile_options(main PRIVATE -g)
#endif()

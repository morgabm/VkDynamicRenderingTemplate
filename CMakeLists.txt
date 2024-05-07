cmake_minimum_required(VERSION 3.26)
project(VkDynamicRenderingTemplate)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory(vk-bootstrap)
add_subdirectory(shaders)


add_executable(VkDynamicRenderingTemplate
        src/main.cpp
        src/dynamic_rendering.cpp
        src/image_tools.cpp
)

target_include_directories(VkDynamicRenderingTemplate PUBLIC
        include
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(VkDynamicRenderingTemplate PUBLIC
        glfw
        ${Vulkan_LIBRARIES}
        vk-bootstrap::vk-bootstrap
)

add_dependencies(VkDynamicRenderingTemplate Shaders)
#pragma once

#include <array>

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

struct DynamicRenderingContext {
    vkb::Instance vkb_instance;
    vkb::PhysicalDevice vkb_physical_device;
    vkb::Device vkb_device;
    vkb::Swapchain vkb_swapchain;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
};

struct DynamicRenderingPipeline {
    VkPipelineLayout layout;
    VkPipeline pipeline;
    std::vector<VkShaderModule> shaderModules;
    std::array<VkFormat, 3> formats;
};

std::pair<VkInstance, vkb::Instance> init_instance();
std::pair<VkPhysicalDevice, vkb::PhysicalDevice> init_physical_device(const vkb::Instance& instance, const VkSurfaceKHR& surface);
std::pair<VkDevice, vkb::Device> init_device(const vkb::PhysicalDevice& physical_device);
std::tuple<VkSwapchainKHR, std::vector<VkImage>, std::vector<VkImageView>, vkb::Swapchain> init_swapchain(const vkb::Device& device);

std::pair<VkCommandPool, VkCommandBuffer> create_primary_command_resources(const DynamicRenderingContext& context);

VkShaderModule create_shader_module(const DynamicRenderingContext& context, const char* filename);

DynamicRenderingPipeline create_dynamic_rendering_pipeline(const DynamicRenderingContext& context, std::vector<VkPipelineShaderStageCreateInfo> shaderStages);

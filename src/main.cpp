//
// Created by Bailey on 5/7/2024.
//

#include "VkBootstrap.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "dynamic_rendering.h"
#include "image_tools.h"


int main() {
    // Initialize glfw for vulkan and create a window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window =
            glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    // Create the rendering context
    DynamicRenderingContext ctx{};
    std::tie(ctx.instance, ctx.vkb_instance) = init_instance();
    glfwCreateWindowSurface(ctx.instance, window, nullptr, &ctx.surface);
    std::tie(ctx.physical_device, ctx.vkb_physical_device)                                      = init_physical_device(ctx.vkb_instance, ctx.surface);
    std::tie(ctx.device, ctx.vkb_device)                                                        = init_device(ctx.vkb_physical_device);
    std::tie(ctx.swapchain, ctx.swapchain_images, ctx.swapchain_image_views, ctx.vkb_swapchain) = init_swapchain(ctx.vkb_device);
    ctx.graphics_queue = ctx.vkb_device.get_queue(vkb::QueueType::graphics).value();

    // Create a command pool and command buffer
    std::tie(ctx.command_pool, ctx.command_buffer) = create_primary_command_resources(ctx);

    // Create the shader modules
    VkShaderModule vertShaderModule = create_shader_module(ctx, "shaders/vertex.vert.spv");
    VkShaderModule fragShaderModule = create_shader_module(ctx, "shaders/fragment.frag.spv");

    // Create the shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    // Create the pipeline
    DynamicRenderingPipeline pipeline = create_dynamic_rendering_pipeline(ctx, {vertShaderStageInfo, fragShaderStageInfo});
    vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragShaderModule, nullptr);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Create semaphore
        VkSemaphore           imageAvailableSemaphore, renderFinishedSemaphore;
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
        vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        // Begin the command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(ctx.command_buffer, &beginInfo);

        insert_memory_barrier(ctx.command_buffer,
                              ctx.swapchain_images[imageIndex],
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.imageView = ctx.swapchain_image_views[imageIndex];
        colorAttachmentInfo.clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;
        renderingInfo.layerCount = 1;
        renderingInfo.viewMask = 0;
        renderingInfo.renderArea.offset = {0, 0};
        renderingInfo.renderArea.extent = ctx.vkb_swapchain.extent;

        vkCmdBeginRendering(ctx.command_buffer, &renderingInfo);
        vkCmdBindPipeline(ctx.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

        //set dynamic viewport and scissor
        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = ctx.vkb_swapchain.extent.width;
        viewport.height = ctx.vkb_swapchain.extent.height;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(ctx.command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = ctx.vkb_swapchain.extent.width;
        scissor.extent.height = ctx.vkb_swapchain.extent.height;
        vkCmdSetScissor(ctx.command_buffer, 0, 1, &scissor);

        vkCmdDraw(ctx.command_buffer, 3, 1, 0, 0);
        vkCmdEndRendering(ctx.command_buffer);

        insert_memory_barrier(ctx.command_buffer,
                              ctx.swapchain_images[imageIndex],
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        vkEndCommandBuffer(ctx.command_buffer);

        // Submit the command buffer
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo         submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &ctx.command_buffer;
        submitInfo.pSignalSemaphores    = &renderFinishedSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        vkQueueSubmit(ctx.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);

        // Present the image
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &renderFinishedSemaphore;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &ctx.swapchain;
        presentInfo.pImageIndices      = &imageIndex;
        vkQueuePresentKHR(ctx.graphics_queue, &presentInfo);

        vkDeviceWaitIdle(ctx.device);
        // Destroy the semaphores
        vkDestroySemaphore(ctx.device, imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(ctx.device, renderFinishedSemaphore, nullptr);

        glfwPollEvents();
    }
}
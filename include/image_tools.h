#pragma once

#include <vulkan/vulkan.h>

VkImageSubresourceRange basic_subresource_range();

VkImageMemoryBarrier insert_memory_barrier(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage);

void clear_image(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f});
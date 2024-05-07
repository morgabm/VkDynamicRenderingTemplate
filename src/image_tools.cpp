#include "image_tools.h"


VkImageSubresourceRange basic_subresource_range() {
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = 1;
    return subresourceRange;
}

VkImageMemoryBarrier insert_memory_barrier(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlagBits srcStage, VkPipelineStageFlagBits dstStage) {
    VkImageSubresourceRange subresourceRange = basic_subresource_range();

    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = 0;
    barrier.dstQueueFamilyIndex = 0;
    barrier.image               = image;
    barrier.subresourceRange    = subresourceRange;

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    return barrier;
}

void clear_image(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColorValue clearColor) {
    VkImageSubresourceRange subresourceRange = basic_subresource_range();
    vkCmdClearColorImage(commandBuffer, image, imageLayout, &clearColor, 1, &subresourceRange);
}
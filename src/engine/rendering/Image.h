#pragma once
#include <vk_mem_alloc.h>


class VulkanEngine;

namespace rendering
{
    class SwapchainRenderer;

    class Image
    {
    public:
        VmaAllocator allocator = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        
        VkDevice device;
        VkImageView imageView;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
        VkImageAspectFlags aspectFlags;
        VkImageLayout currentLayout;

        static Image Allocate(VmaAllocator allocator, VkDevice device, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, bool mipmapped = false);

        void Write(const SwapchainRenderer* swapchain, const void* data);

        void Transition(VkCommandBuffer cmd, VkImageLayout newLayout);
        void BlitCopyTo(VkCommandBuffer cmd, Image& dst);

        void Destroy();
    };
}

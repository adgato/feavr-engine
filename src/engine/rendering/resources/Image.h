#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

class VulkanEngine;

namespace rendering
{
    class EngineResources;

    class Image
    {
    public:
        EngineResources* renderer;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        
        VkImageView imageView;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
        VkImageAspectFlags aspectFlags;
        VkImageLayout currentLayout;

        static Image Allocate(EngineResources* renderer, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, bool mipmapped = false);

        void Write(const void* data);

        void Transition(VkCommandBuffer cmd, VkImageLayout newLayout);
        void BlitCopyTo(VkCommandBuffer cmd, Image& dst);

        void Destroy();
    };
}

#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

class RenderingEngine;

namespace rendering
{
    class RenderingResources;

    template <typename T>
    class Buffer;

    class Image
    {
    public:
        RenderingResources* renderer;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        VkImageView imageView;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
        VkImageAspectFlags aspectFlags;
        VkImageLayout currentLayout;

        static Image Allocate(RenderingResources* renderer, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, bool mipmapped = false);

        void WriteSampled(const void* data);

        void Read(VkCommandBuffer cmd, VkBuffer intoBuffer, VkMemoryBarrier2& srcBarrier);

        void ReadFromRenderTarget(const VkCommandBuffer cmd, const VkBuffer intoBuffer)
        {
            VkMemoryBarrier2 barrier;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            Read(cmd, intoBuffer, barrier);
        }

        void Barrier(VkCommandBuffer cmd, VkImageLayout newLayout, const VkMemoryBarrier2& barrier);/* = {
                         VK_STRUCTURE_TYPE_MEMORY_BARRIER_2, nullptr,
                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                         VK_ACCESS_2_MEMORY_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                         VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
                     });*/

        void BlitCopyTo(VkCommandBuffer cmd, Image& dst, VkMemoryBarrier2& srcBarrier);

        void BlitFromRenderTarget(const VkCommandBuffer cmd, Image& dst)
        {
            VkMemoryBarrier2 barrier;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            BlitCopyTo(cmd, dst, barrier);
        }

        void Destroy();
    };
}

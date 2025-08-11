#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

class VulkanEngine;

namespace rendering
{
    class EngineResources;

    template <typename T>
    class Buffer;

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

        void Read(const Buffer<std::byte>& intoBuffer);

        void Barrier(VkCommandBuffer cmd, VkImageLayout newLayout, const VkMemoryBarrier2& barrier = {
                         VK_STRUCTURE_TYPE_MEMORY_BARRIER_2, nullptr,
                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                         VK_ACCESS_2_MEMORY_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                         VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
                     });

        void BlitCopyTo(VkCommandBuffer cmd, Image& dst);

        void Destroy();
    };
}

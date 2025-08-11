#include "Image.h"


#include "vk_mem_alloc.h"

#include "rendering/VulkanEngine.h"
#include "rendering/utility/Initializers.h"
#include "rendering/utility/VulkanNew.h"
#include "rendering/resources/EngineResources.h"

namespace rendering
{
    Image Image::Allocate(EngineResources* renderer, const VkExtent3D size, const VkFormat format,
                          const VkImageUsageFlags usage, const VkImageAspectFlags aspectFlags /*= VK_IMAGE_ASPECT_COLOR_BIT*/, const bool mipmapped /*= false*/)
    {
        Image newImage;
        newImage.renderer = renderer;
        newImage.imageFormat = format;
        newImage.imageExtent = size;
        newImage.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        newImage.aspectFlags = aspectFlags;

        auto img_info = vkinit::New<VkImageCreateInfo>(); {
            img_info.imageType = VK_IMAGE_TYPE_2D;

            img_info.format = format;
            img_info.extent = size;

            img_info.mipLevels = 1;
            img_info.arrayLayers = 1;

            //for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
            img_info.samples = VK_SAMPLE_COUNT_1_BIT;

            //optimal tiling, which means the image is stored on the best gpu format
            img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            img_info.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (mipmapped)
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;

        // always allocate images on dedicated GPU memory
        VmaAllocationCreateInfo allocinfo = {};
        allocinfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VmaAllocationInfo info;
        VK_CHECK(vmaCreateImage(renderer->resource, &img_info, &allocinfo, &newImage.image, &newImage.allocation, &info));

        // build a image-view for the depth image to use for rendering
        auto view_info = vkinit::New<VkImageViewCreateInfo>(); {
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.image = newImage.image;
            view_info.format = format;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;
            view_info.subresourceRange.aspectMask = aspectFlags;
        }
        view_info.subresourceRange.levelCount = img_info.mipLevels;

        VK_CHECK(vkCreateImageView(renderer->resource, &view_info, nullptr, &newImage.imageView));

        return newImage;
    }

    void Image::Write(const void* data)
    {
        const size_t data_size = static_cast<size_t>(imageExtent.depth) * imageExtent.width * imageExtent.height * 4;

        Buffer<std::byte> uploadBuffer = Buffer<std::byte>::Allocate(renderer->resource, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HostAccess::SEQUENTIAL_WRITE);
        uploadBuffer.Write(static_cast<const std::byte*>(data));

        renderer->ImmediateSumbit([&](const VkCommandBuffer cmd)
        {
            Barrier(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = aspectFlags;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;

            // copy the buffer into the image
            vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            Barrier(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        uploadBuffer.Destroy();
    }

    void Image::Read(const Buffer<std::byte>& intoBuffer)
    {
        renderer->ImmediateSumbit([&](const VkCommandBuffer cmd)
        {
            Barrier(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = aspectFlags;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;

            // copy the buffer into the image
            vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, intoBuffer.buffer, 1, &copyRegion);
        });
    }

    void Image::Barrier(const VkCommandBuffer cmd, const VkImageLayout newLayout, const VkMemoryBarrier2& barrier)
    {
        auto imageBarrier = vkinit::New<VkImageMemoryBarrier2>(); {
            imageBarrier.srcStageMask = barrier.srcStageMask;
            imageBarrier.srcAccessMask = barrier.srcAccessMask;
            imageBarrier.dstStageMask = barrier.dstStageMask;
            imageBarrier.dstAccessMask = barrier.dstAccessMask;

            imageBarrier.oldLayout = currentLayout;
            imageBarrier.newLayout = newLayout;

            imageBarrier.subresourceRange.aspectMask = aspectFlags;
            imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            imageBarrier.image = image;
        }

        auto depInfo = vkinit::New<VkDependencyInfo>(); {
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &imageBarrier;
        }

        vkCmdPipelineBarrier2(cmd, &depInfo);

        currentLayout = newLayout;
    }

    void Image::BlitCopyTo(const VkCommandBuffer cmd, Image& dst)
    {
        Barrier(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        dst.Barrier(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        auto blitRegion = vkinit::New<VkImageBlit2>(); {
            blitRegion.srcOffsets[1] = { static_cast<int32_t>(imageExtent.width), static_cast<int32_t>(imageExtent.height), static_cast<int32_t>(imageExtent.depth) };
            blitRegion.dstOffsets[1] = { static_cast<int32_t>(dst.imageExtent.width), static_cast<int32_t>(dst.imageExtent.height), static_cast<int32_t>(dst.imageExtent.depth) };

            blitRegion.srcSubresource.aspectMask = aspectFlags;
            blitRegion.srcSubresource.layerCount = 1;

            blitRegion.dstSubresource.aspectMask = dst.aspectFlags;
            blitRegion.dstSubresource.layerCount = 1;
        }

        auto blitInfo = vkinit::New<VkBlitImageInfo2>(); {
            blitInfo.dstImage = dst.image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;
        }

        vkCmdBlitImage2(cmd, &blitInfo);
    }

    void Image::Destroy()
    {
        if (image == VK_NULL_HANDLE)
            return;

        vkDestroyImageView(renderer->resource, imageView, nullptr);

        if (allocation != VK_NULL_HANDLE)
            vmaDestroyImage(renderer->resource, image, allocation);

        image = VK_NULL_HANDLE;
    }
}

#include "Image.h"

#include "Swapchain.h"
#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_new.h"

namespace rendering
{
    Image Image::Allocate(const VmaAllocator allocator, const VkDevice device, const VkExtent3D size, const VkFormat format, const VkImageUsageFlags usage, const VkImageAspectFlags aspectFlags /*= VK_IMAGE_ASPECT_COLOR_BIT*/, const bool mipmapped /*= false*/)
    {
        Image newImage;
        newImage.allocator = allocator;
        newImage.device = device;
        newImage.imageFormat = format;
        newImage.imageExtent = size;
        newImage.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        newImage.aspectFlags = aspectFlags;

        VkImageCreateInfo img_info = vkinit::image_create_info(format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, size);
        if (mipmapped)
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;

        // always allocate images on dedicated GPU memory
        VmaAllocationCreateInfo allocinfo = {};
        allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(newImage.allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

        VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlags);
        view_info.subresourceRange.levelCount = img_info.mipLevels;

        VK_CHECK(vkCreateImageView(newImage.device, &view_info, nullptr, &newImage.imageView));
        
        return newImage;
    }

    void Image::Write(const SwapchainRenderer* swapchain, const void* data)
    {
        const size_t data_size = static_cast<size_t>(imageExtent.depth) * imageExtent.width * imageExtent.height * 4;

        const RawBuffer uploadBuffer = RawBuffer::Allocate(allocator, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        uploadBuffer.Write(data);

        swapchain->ImmediateSumbit([&](const VkCommandBuffer cmd)
        {
            Transition(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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

            Transition(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        uploadBuffer.Destroy();
    }

    void Image::Transition(const VkCommandBuffer cmd, const VkImageLayout newLayout)
    {
        auto imageBarrier = vkinit::New<VkImageMemoryBarrier2>();
        {
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

            imageBarrier.oldLayout = currentLayout;
            imageBarrier.newLayout = newLayout;
        
            imageBarrier.subresourceRange.aspectMask = aspectFlags;
            imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            
            imageBarrier.image = image;
        }

        auto depInfo = vkinit::New<VkDependencyInfo>();
        {
            depInfo.imageMemoryBarrierCount = 1;
            depInfo.pImageMemoryBarriers = &imageBarrier;
        }

        vkCmdPipelineBarrier2(cmd, &depInfo);

        currentLayout = newLayout;
    }

    void Image::BlitCopyTo(const VkCommandBuffer cmd, Image& dst)
    {
        Transition(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        dst.Transition(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        
        auto blitRegion = vkinit::New<VkImageBlit2>();
        {
            blitRegion.srcOffsets[1] = { static_cast<int32_t>(imageExtent.width), static_cast<int32_t>(imageExtent.height), static_cast<int32_t>(imageExtent.depth) };
            blitRegion.dstOffsets[1] = { static_cast<int32_t>(dst.imageExtent.width), static_cast<int32_t>(dst.imageExtent.height), static_cast<int32_t>(dst.imageExtent.depth) };

            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.layerCount = 1;

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.layerCount = 1;
        }
	
        auto blitInfo = vkinit::New<VkBlitImageInfo2>();
        {
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
        
        vkDestroyImageView(device, imageView, nullptr);

        if (allocation != VK_NULL_HANDLE)
            vmaDestroyImage(allocator, image, allocation);

        image = VK_NULL_HANDLE;
    }
}

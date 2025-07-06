// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

namespace rendering
{
    class Image;
}

namespace vkinit
{
    
    VkRenderingAttachmentInfo attachment_info(const rendering::Image& view, const VkClearValue* clear);
    VkRenderingInfo rendering_info(VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colorAttachment, const VkRenderingAttachmentInfo* depthAttachment);
    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
}

// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <vulkan/vulkan_core.h>

namespace rendering
{
    class Image;
}

namespace vkinit
{
    
    VkRenderingAttachmentInfo attachment_info(const rendering::Image& view, const VkClearValue* clear);
    VkRenderingInfo rendering_info(VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colorAttachment, const VkRenderingAttachmentInfo* depthAttachment);
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
}

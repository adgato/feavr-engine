#include "rendering/utility/Initializers.h"

#include "rendering/utility/VulkanNew.h"
#include "rendering/resources/Image.h"

VkRenderingAttachmentInfo vkinit::attachment_info(const rendering::Image& view, const VkClearValue* clear)
{
    auto colorAttachment = vkinit::New<VkRenderingAttachmentInfo>(); {
        colorAttachment.imageView = view.imageView;
        colorAttachment.imageLayout = view.currentLayout;
        colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    if (clear)
        colorAttachment.clearValue = *clear;

    return colorAttachment;
}

VkRenderingInfo vkinit::rendering_info(const VkExtent2D renderExtent, const VkRenderingAttachmentInfo* colorAttachment, const VkRenderingAttachmentInfo* depthAttachment)
{
    VkRenderingInfo renderInfo {};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

    renderInfo.renderArea = VkRect2D { VkOffset2D { 0, 0 }, renderExtent };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = colorAttachment;
    renderInfo.pDepthAttachment = depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    return renderInfo;
}

VkImageViewCreateInfo vkinit::imageview_create_info(const VkFormat format, const VkImage image, const VkImageAspectFlags aspectFlags)
{
    // build a image-view for the depth image to use for rendering
    auto info = vkinit::New<VkImageViewCreateInfo>(); {
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = image;
        info.format = format;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.aspectMask = aspectFlags;
    }

    return info;
}

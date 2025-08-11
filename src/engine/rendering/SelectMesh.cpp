#include "SelectMesh.h"

#include "pass-system/PassMeshManager.h"
#include "resources/EngineResources.h"
#include "utility/Initializers.h"
#include "utility/Screenshot.h"
#include "pass-system/PassDirectory.h"
#include "pass-system/ScreenRaycastPass.h"

namespace rendering
{
    void SelectMesh(EngineResources& resources, PassMeshManager& passManager, const Image& drawTemplate, const Image& depthTemplate)
    {

        VkImageUsageFlags drawImageUsages {};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageUsageFlags depthImageUsages {};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        Image drawImage = Image::Allocate(&resources, drawTemplate.imageExtent, drawTemplate.imageFormat, drawImageUsages, drawTemplate.aspectFlags);
        Image depthImage = Image::Allocate(&resources, depthTemplate.imageExtent, depthTemplate.imageFormat, depthImageUsages, depthTemplate.aspectFlags);

        resources.ImmediateSumbit([&](const VkCommandBuffer cmd)
        {
            VkClearValue clearColor;
            clearColor.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

            VkClearValue clearDepth;
            clearDepth.depthStencil.depth = 0.0f;

            drawImage.Barrier(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            depthImage.Barrier(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

            const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(drawImage, &clearColor);
            const VkRenderingAttachmentInfo depthAttachment = vkinit::attachment_info(depthImage, &clearDepth);

            VkRect2D scissor = {};
            scissor.extent = { drawImage.imageExtent.width, drawImage.imageExtent.height };

            const VkRenderingInfo renderInfo = vkinit::rendering_info(scissor.extent, &colorAttachment, &depthAttachment);

            vkCmdBeginRendering(cmd, &renderInfo);
            //set dynamic viewport and scissor

            VkViewport viewport = {};
            viewport.width = static_cast<float>(drawImage.imageExtent.width);
            viewport.height = static_cast<float>(drawImage.imageExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            passManager.GetPass<unlit_pass::Pass>().Draw(cmd, passManager.GetMeshes());

            vkCmdEndRendering(cmd);
        });

        utility::Screenshot(resources.resource, PROJECT_ROOT"/test.png", drawImage);

        drawImage.Destroy();
        depthImage.Destroy();
    }
}

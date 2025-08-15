#include "ClickOnMeshTool.h"

#include "hlsl++/vector_int.h"
#include "rendering/pass-system/PassMeshManager.h"
#include "Initializers.h"
#include "hlsl++/vector_uint.h"
#include "rendering/pass-system/PassDirectory.h"
#include "rendering/pass-system/IdentifyPass.h"

namespace rendering
{
    ClickOnMeshTool::ClickOnMeshTool(EngineResources& engineResources, PassMeshManager& passManager)
        : resources(engineResources), passManager(passManager) {}

    void ClickOnMeshTool::Init(const VkExtent3D extent)
    {
        drawImage = Image::Allocate(&resources, extent, VK_FORMAT_R32_UINT,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        depthImage = Image::Allocate(&resources, extent, VK_FORMAT_D32_SFLOAT,
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    void ClickOnMeshTool::DrawMeshIndices(VkCommandBuffer cmd)
    {
        selectFrame = resources.frameCount;
        VkClearValue clearColor;
        clearColor.color.uint32[0] = ecs::BadMaxIndex; // bad entity when clicking on background

        VkClearValue clearDepth;
        clearDepth.depthStencil.depth = 0.0f;

        // barrier to make sure the image has finished its previous rendering and blitted to swapchain
        VkMemoryBarrier2 drawBarrier;
        drawBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        drawBarrier.srcAccessMask = VK_ACCESS_2_NONE;
        drawBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        drawBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

        // don't want to clear before its finished the previous draw
        VkMemoryBarrier2 depthBarrier;
        depthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        depthBarrier.srcAccessMask = VK_ACCESS_2_NONE;
        depthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        depthBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        drawImage.Barrier(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, drawBarrier);
        depthImage.Barrier(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depthBarrier);

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
    }

    bool ClickOnMeshTool::Destroyed() const
    {
        return drawImage.image == VK_NULL_HANDLE;
    }

    bool ClickOnMeshTool::DrawCompleted(const uint32_t sufficientFrameDelta /* = 1*/) const
    {
        return !Destroyed() && resources.frameCount >= selectFrame + sufficientFrameDelta;
    }

    uint32_t ClickOnMeshTool::SampleCoordinate(const hlslpp::int2& coord)
    {
        using namespace hlslpp;

        const size_t pixelCount = static_cast<size_t>(drawImage.imageExtent.depth) * drawImage.imageExtent.width * drawImage.imageExtent.height;
        Buffer<uint32_t> buffer = Buffer<uint32_t>::Allocate(resources.resource, pixelCount,
                                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT, HostAccess::RANDOM, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        drawImage.ReadFromRenderTarget(buffer.buffer);

        const int2 clampedCoord = clamp(coord, int2(0, 0), int2(drawImage.imageExtent.width, drawImage.imageExtent.height));
        const uint32_t id = buffer.Access()[clampedCoord.y * int1(drawImage.imageExtent.width) + clampedCoord.x];
        buffer.Destroy();
        return id;
    }

    void ClickOnMeshTool::Destroy()
    {
        drawImage.Destroy();
        depthImage.Destroy();
    }
}

#include "ClickOnMeshTool.h"

#include "hlsl++/vector_int.h"
#include "rendering/pass-system/PassSystem.h"
#include "Initializers.h"
#include "hlsl++/vector_uint.h"
#include "rendering/Camera.h"
#include "rendering/Camera.h"
#include "rendering/pass-system/PassDirectory.h"
#include "rendering/pass-system/IdentifyPass.h"

namespace rendering
{
    ClickOnMeshTool::ClickOnMeshTool(RenderingResources& engineResources, PassSystem& passManager)
        : resources(engineResources), passManager(passManager) {}

    void ClickOnMeshTool::Init()
    {
        constexpr VkExtent3D extent = {1, 1, 1};
        drawImage = Image::Allocate(&resources, extent, VK_FORMAT_R32_UINT,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        depthImage = Image::Allocate(&resources, extent, VK_FORMAT_D32_SFLOAT,
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        resultBuffer = Buffer<uint32_t>::Allocate(resources.resource, 1, VK_BUFFER_USAGE_TRANSFER_DST_BIT, HostAccess::RANDOM, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    }

    void ClickOnMeshTool::DrawMeshIndices(VkCommandBuffer_T* cmd, const glm::mat4& cameraView, const hlslpp::int2& coord)
    {
        this->coord = coord;

        waitingSample = true;
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
        scissor.offset = { 0, 0 };
        scissor.extent = { drawImage.imageExtent.width, drawImage.imageExtent.height };

        const VkRenderingInfo renderInfo = vkinit::rendering_info({ drawImage.imageExtent.width, drawImage.imageExtent.height }, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(cmd, &renderInfo);
        //set dynamic viewport and scissor

        VkViewport viewport = {};
        viewport.width = static_cast<float>(drawImage.imageExtent.width);
        viewport.height = static_cast<float>(drawImage.imageExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Original frustum dimensions at near plane
        float fovy = glm::radians(70.0f);
        float near = 10000.f;
        float far = 0.1f;
        float aspect = static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height);
        float near_height = 2.0f * near * glm::tan(fovy / 2.0f);
        float near_width = near_height * aspect;

        // Size of one pixel at the near plane
        float pixel_width = near_width / windowSize.width;
        float pixel_height = near_height / windowSize.height;

        // Now calculate the frustum bounds
        float center_x = (2.0f * coord.x / windowSize.width - 1.0f) * (near_width / 2.0f);
        float center_y = (2.0f * coord.y / windowSize.height - 1.0f) * (near_height / 2.0f);

        float left = center_x - pixel_width / 2.0f;
        float right = center_x + pixel_width / 2.0f;
        float bottom = center_y - pixel_height / 2.0f;
        float top = center_y + pixel_height / 2.0f;

        glm::mat4 projection = glm::frustum(left, right, bottom, top, near, far);
        projection[1][1] *= -1;

        GlobalSceneData sceneData;
        sceneData.proj = projection;
        sceneData.view = cameraView;
        sceneData.viewproj = sceneData.proj * sceneData.view;
        passManager.GetPass<identify_pass::Pass>().Draw(cmd, passManager.GetMeshes(), selectFrame, sceneData);

        vkCmdEndRendering(cmd);

        drawImage.ReadFromRenderTarget(cmd, resultBuffer.buffer);
    }

    bool ClickOnMeshTool::DrawWaitingSample(const uint32_t sufficientFrameDelta /* = 1*/) const
    {
        return waitingSample && resources.frameCount >= selectFrame + sufficientFrameDelta;
    }

    uint32_t ClickOnMeshTool::SampleCoordinate()
    {
        waitingSample = false;
        return resultBuffer.Access()[0];
    }

    void ClickOnMeshTool::Destroy()
    {
        resultBuffer.Destroy();
        drawImage.Destroy();
        depthImage.Destroy();
    }
}

#include "RenderingEngine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include "VkBootstrap.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <tuple>


#include <glm/gtx/transform.hpp>

#include "assets-system/AssetManager.h"
#include "widgets/EngineWidget.h"
#include "ecs/EngineView.h"
#include "fmt/ranges.h"
#include "rendering/utility/Initializers.h"
#include "rendering/resources/Image.h"
#include "pass-system/shader_descriptors.h"
#include "resources/RenderingResources.h"
#include "utility/Screenshot.h"

using namespace rendering;

RenderingEngine::RenderingEngine(ecs::Engine& engine, RenderingResources& resources)
    : resources(resources), resource(resources.resource), passSys(resources, *this, engine), defaultMaterial(engine, passSys) {}

void RenderingEngine::Init(RenderingResources& resources)
{
    // We initialize SDL and create a window with it.

    //depth image size will match the window
    constexpr VkExtent2D windowSize { 1920, 1080 };
    const VkExtent3D drawImageExtent = { windowSize.width, windowSize.height, 1 };

    VkImageUsageFlags drawImageUsages {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageUsageFlags depthImageUsages {};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    drawImage = Image::Allocate(&resources, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);
    depthImage = Image::Allocate(&resources, drawImageExtent, VK_FORMAT_D24_UNORM_S8_UINT, depthImageUsages, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    commonTextures.Init(&resources);
    commonSets.Init(resource);

    for (auto& [frameDescriptors, sceneDataBuffer] : frameData)
        frameDescriptors.Init(resource);

    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(resource, &sampl, nullptr, &defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(resource, &sampl, nullptr, &defaultSamplerLinear);

    passSys.Init();

    mainCamera.velocity = glm::vec3(0.f);
    mainCamera.position = glm::vec3(30.f, -00.f, -085.f);

    mainCamera.pitch = 0;
    mainCamera.yaw = 0;
}

void RenderingEngine::Draw(const uint32_t frameCount, VkCommandBuffer cmd, Image& targetImage)
{
    // optional, quite costly when resizing
    if (drawImage.imageExtent.width != targetImage.imageExtent.width || drawImage.imageExtent.height != targetImage.imageExtent.height)
    {
        drawImage.Destroy();
        depthImage.Destroy();
        VkImageUsageFlags drawImageUsages {};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageUsageFlags depthImageUsages {};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        drawImage = Image::Allocate(&resources, targetImage.imageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);
        depthImage = Image::Allocate(&resources, targetImage.imageExtent, VK_FORMAT_D24_UNORM_S8_UINT, depthImageUsages, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    }

    VkClearValue clearColor;
    clearColor.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkClearValue clearDepth;
    clearDepth.depthStencil.depth = 0.0f;
    clearDepth.depthStencil.stencil = 0;

    VkMemoryBarrier2 drawBarrier;
    drawBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    drawBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    drawBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    drawBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    VkMemoryBarrier2 depthBarrier;
    depthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    depthBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    depthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    depthBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    drawImage.Barrier(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, drawBarrier);
    depthImage.Barrier(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthBarrier);

    const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(drawImage, &clearColor);
    const VkRenderingAttachmentInfo depthAttachment = vkinit::attachment_info(depthImage, &clearDepth);

    const VkExtent2D drawImageExtent = { drawImage.imageExtent.width, drawImage.imageExtent.height };

    const VkRenderingInfo renderInfo = vkinit::rendering_info(drawImageExtent, &colorAttachment, &depthAttachment, true);

    vkCmdBeginRendering(cmd, &renderInfo);
    //set dynamic viewport and scissor
    VkViewport viewport = {};
    viewport.width = static_cast<float>(drawImageExtent.width);
    viewport.height = static_cast<float>(drawImageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.extent = drawImageExtent;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    //allocate a new uniform buffer for the scene data

    auto& [frameDescriptors, sceneDataBuffer] = frameData[frameCount % FRAME_OVERLAP];
    sceneDataBuffer.Destroy();
    frameDescriptors.ResetPools();
    mainCamera.SetAspect(static_cast<float>(targetImage.imageExtent.width) / static_cast<float>(targetImage.imageExtent.height));
    mainCamera.Update();

    sceneData.view = mainCamera.view;
    sceneData.proj = mainCamera.proj;
    sceneData.viewproj = sceneData.proj * sceneData.view;
    sceneDataBuffer = Buffer<GlobalSceneData>::Allocate(resource, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HostAccess::SEQUENTIAL_WRITE);
    sceneDataBuffer.Write(&sceneData);

    //create a descriptor set that binds that buffer and update it
    sceneProperties.AllocateSet(resource, *resource.globalAllocator, commonSets.sceneDataLayout);
    sceneProperties.StageBuffer(shader_layouts::global::SceneData_binding, sceneDataBuffer);
    sceneProperties.PerformWrites();

    passSys.Draw(cmd);
    vkCmdEndRendering(cmd);

    drawImage.BlitFromRenderTarget(cmd, targetImage);
}

void RenderingEngine::Destroy()
{
    //make sure the gpu has stopped doing its things

    for (auto& [frameDescriptors, sceneDataBuffer] : frameData)
    {
        sceneDataBuffer.Destroy();
        frameDescriptors.DestroyPools();
    }

    passSys.Destroy();

    commonTextures.Destroy();

    vkDestroySampler(resource, defaultSamplerNearest, nullptr);
    vkDestroySampler(resource, defaultSamplerLinear, nullptr);

    depthImage.Destroy();
    drawImage.Destroy();

    commonSets.Destroy();
}

//< upload_image

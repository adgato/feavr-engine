#include "VulkanEngine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include "VkBootstrap.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <tuple>


#include <glm/gtx/transform.hpp>

#include "assets-system/AssetManager.h"
#include "ecs/EngineExtensions.h"
#include "ecs/EngineView.h"
#include "fmt/ranges.h"
#include "rendering/utility/Initializers.h"
#include "rendering/resources/Image.h"
#include "pass-system/shader_descriptors.h"
#include "resources/EngineResources.h"
#include "utility/Screenshot.h"

using namespace rendering;

ResourceHandles& VulkanEngine::Resource() const { return engineResources->resource; }

void VulkanEngine::Init(EngineResources* swapchainRenderer)
{
    // We initialize SDL and create a window with it.

    this->engineResources = swapchainRenderer;

    //depth image size will match the window
    constexpr VkExtent3D drawImageExtent = { windowSize.width, windowSize.height, 1 };

    VkImageUsageFlags drawImageUsages {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageUsageFlags depthImageUsages {};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    drawImage = Image::Allocate(swapchainRenderer, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);
    depthImage = Image::Allocate(swapchainRenderer, drawImageExtent, VK_FORMAT_D24_UNORM_S8_UINT, depthImageUsages, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    commonTextures.Init(swapchainRenderer);
    commonSets.Init(Resource());

    for (auto& [frameDescriptors, sceneDataBuffer] : frameData)
        frameDescriptors.Init(Resource());

    VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(Resource(), &sampl, nullptr, &defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(Resource(), &sampl, nullptr, &defaultSamplerLinear);

    passManager.Init(this);

    mainCamera.velocity = glm::vec3(0.f);
    mainCamera.position = glm::vec3(30.f, -00.f, -085.f);

    mainCamera.pitch = 0;
    mainCamera.yaw = 0;
}

void VulkanEngine::Draw(const uint32_t frameCount, VkCommandBuffer cmd, Image& targetImage)
{
    mainCamera.update();

    const glm::mat4 view = mainCamera.getViewMatrix();

    // camera projection
    glm::mat4 projection = glm::perspective(glm::radians(70.f), static_cast<float>(windowSize.width) / static_cast<float>(windowSize.height), 10000.f, 0.1f);

    // invert the Y direction on projection matrix so that we are more similar
    // to opengl and gltf axis
    projection[1][1] *= -1;

    sceneData.view = view;
    sceneData.proj = projection;
    sceneData.viewproj = projection * view;

    sceneData.ambientColor = glm::vec4(.1f);
    sceneData.sunlightColor = glm::vec4(1.f);
    sceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

    //begin a render pass  connected to our draw image

    auto& [frameDescriptors, sceneDataBuffer] = frameData[frameCount % FRAME_OVERLAP];
    sceneDataBuffer.Destroy();
    frameDescriptors.ResetPools();

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
    sceneDataBuffer = Buffer<GPUSceneData>::Allocate(Resource(), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HostAccess::SEQUENTIAL_WRITE);
    sceneDataBuffer.Write(&sceneData);

    //create a descriptor set that binds that buffer and update it
    gpuSceneDescriptorSet.AllocateSet(Resource(), commonSets.GPUSceneData);
    gpuSceneDescriptorSet.StageBuffer(shader_layouts::global::SceneData_binding, sceneDataBuffer);
    gpuSceneDescriptorSet.PerformWrites();

    passManager.Draw(cmd);
    vkCmdEndRendering(cmd);

    drawImage.BlitFromRenderTarget(cmd, targetImage);
}

void VulkanEngine::SaveScene(const char* filePath)
{
    serial::Stream m;
    m.InitWrite();
    passManager.Serialize(m);

    ecsEngine.Refresh();
    ecsEngine.Serialize(m);

    assets_system::AssetFile saveAsset("SCNE", 0);

    saveAsset.header["Meshes Start"] = static_cast<uint64_t>(passManager.serializeInfo.meshesStart);
    saveAsset.header["Meshes Size"] = static_cast<uint64_t>(passManager.serializeInfo.meshesSizeBytes);

    saveAsset.WriteToBlob(m);
    saveAsset.Save(filePath);
}

void VulkanEngine::LoadScene(const assets_system::AssetID other)
{
    const assets_system::AssetFile loadAsset = assets_system::AssetManager::LoadAsset(other);
    assert(loadAsset.HasFormat("SCNE", 0));

    serial::Stream m = loadAsset.ReadFromBlob();

    passManager.Destroy();
    ecsEngine.Destroy();
    passManager.Init(this);
    passManager.Serialize(m);
    ecsEngine.Serialize(m);
    ecsEngine.Refresh();
    passManager.ReplaceInvalidAssetSources(other);
    passManager.FixupReferences(ecsEngine);
}

void VulkanEngine::Destroy()
{
    //make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(Resource());

    for (auto& [frameDescriptors, sceneDataBuffer] : frameData)
    {
        sceneDataBuffer.Destroy();
        frameDescriptors.DestroyPools();
    }

    passManager.Destroy();
    ecsEngine.Destroy();

    commonTextures.Destroy();

    vkDestroySampler(Resource(), defaultSamplerNearest, nullptr);
    vkDestroySampler(Resource(), defaultSamplerLinear, nullptr);

    depthImage.Destroy();
    drawImage.Destroy();

    commonSets.Destroy();
}

//< upload_image

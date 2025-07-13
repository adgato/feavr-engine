#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#define VMA_IMPLEMENTATION
#include <tuple>

#include "vk_mem_alloc.h"
#include "vk_descriptors.h"
#include "vk_loader.h"

#include <glm/gtx/transform.hpp>

#include "rendering/Buffer.h"
#include "rendering/Image.h"
#include "rendering/shader_descriptors.h"

using namespace rendering;

void VulkanEngine::Init(SwapchainRenderer* swapchainRenderer)
{
    // We initialize SDL and create a window with it. 

    const DeviceData& deviceData = swapchainRenderer->deviceData;
    device = swapchainRenderer->deviceData.device;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = deviceData.physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = deviceData.instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

    //depth image size will match the window
    constexpr VkExtent3D drawImageExtent = {windowSize.width, windowSize.height, 1};

    VkImageUsageFlags drawImageUsages {};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageUsageFlags depthImageUsages {};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    drawImage = Image::Allocate(vmaAllocator, device, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);
    depthImage = Image::Allocate(vmaAllocator, device, drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, VK_IMAGE_ASPECT_DEPTH_BIT);

    descriptorAllocator.Init(device);

    commonTextures.Init(this, swapchainRenderer);
    commonSets.Init(device);

    for (auto& [frameDescriptors, sceneDataBuffer] : frameData)
        frameDescriptors.Init(device);

    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(device, &sampl, nullptr, &defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(device, &sampl, nullptr, &defaultSamplerLinear);


    passManager.Init(this, &entityManager, {
                         {GetTypeID<default_pass::Pass>(), ~0u}
                     });

    defaultMaterial = Material<default_pass::Pass>::First(&passManager);

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
    clearColor.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkClearValue clearDepth;
    clearDepth.depthStencil.depth = 0.0f;

    drawImage.Transition(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    depthImage.Transition(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(drawImage, &clearColor);
    const VkRenderingAttachmentInfo depthAttachment = vkinit::attachment_info(depthImage, &clearDepth);

    const VkExtent2D drawImageExtent = {drawImage.imageExtent.width, drawImage.imageExtent.height};

    const VkRenderingInfo renderInfo = vkinit::rendering_info(drawImageExtent, &colorAttachment, &depthAttachment);

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
    sceneDataBuffer = Buffer<GPUSceneData>::Allocate(vmaAllocator, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    sceneDataBuffer.Write(&sceneData);

    //create a descriptor set that binds that buffer and update it
    gpuSceneDescriptorSet.AllocateSet(frameDescriptors, commonSets.GPUSceneData);
    gpuSceneDescriptorSet.StageBuffer(shader_layouts::global::SceneData_binding, sceneDataBuffer);
    gpuSceneDescriptorSet.PerformWrites();

    passManager.Draw(cmd, ~0u);
    vkCmdEndRendering(cmd);

    drawImage.BlitCopyTo(cmd, targetImage);
}

void VulkanEngine::Destroy()
{
    //make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(device);

    for (auto& frame : frameData)
    {
        frame.sceneDataBuffer.Destroy();
        frame.frameDescriptors.DestroyPools();
    }

    entityManager.Destroy();

    commonTextures.Destroy();
    passManager.Destroy();

    vkDestroySampler(device, defaultSamplerNearest, nullptr);
    vkDestroySampler(device, defaultSamplerLinear, nullptr);

    depthImage.Destroy();
    drawImage.Destroy();

    commonSets.Destroy();

    vmaDestroyAllocator(vmaAllocator);

    descriptorAllocator.DestroyPools();
}

//< upload_image
Mesh VulkanEngine::UploadMesh(const SwapchainRenderer* swapchainRenderer, const std::span<uint32_t> indices, const std::span<Vertex> vertices) const
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    Mesh newSurface;

    //create vertex buffer
    newSurface.vertexBuffer = Buffer<Vertex>::Allocate(vmaAllocator, vertices.size(),
                                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                       VMA_MEMORY_USAGE_GPU_ONLY);

    //find the adress of the vertex buffer
    const VkBufferDeviceAddressInfo deviceAdressInfo {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer};
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

    //create index buffer
    newSurface.indexBuffer = Buffer<uint32_t>::Allocate(vmaAllocator, indices.size(),
                                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                        VMA_MEMORY_USAGE_GPU_ONLY);

    const RawBuffer staging = RawBuffer::Allocate(vmaAllocator, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    staging.Access([&](void* data)
    {
        memcpy(data, vertices.data(), vertexBufferSize);
        memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);
    });

    swapchainRenderer->ImmediateSumbit([&](VkCommandBuffer cmd)
    {
        VkBufferCopy vertexCopy;
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy;
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    });

    staging.Destroy();

    return newSurface;
}

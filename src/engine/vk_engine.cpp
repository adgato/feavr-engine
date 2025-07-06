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

void VulkanEngine::init(SwapchainRenderer* swapchainRenderer)
{
    // We initialize SDL and create a window with it. 

    init_swapchain(swapchainRenderer->deviceData);
    commonTextures.Init(this, swapchainRenderer);
    init_descriptors();
    init_imgui(swapchainRenderer->deviceData);

    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    sampl.magFilter = VK_FILTER_NEAREST;
    sampl.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(device, &sampl, nullptr, &_defaultSamplerNearest);

    sampl.magFilter = VK_FILTER_LINEAR;
    sampl.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(device, &sampl, nullptr, &_defaultSamplerLinear);


    passManager.Init(this, &entityManager, {
                         {GetTypeID<default_pass::Pass>(), ~0u}
                     });

    defaultMaterial = Material<default_pass::Pass>::First(&passManager);

    mainCamera.velocity = glm::vec3(0.f);
    mainCamera.position = glm::vec3(30.f, -00.f, -085.f);

    mainCamera.pitch = 0;
    mainCamera.yaw = 0;

    //everything went fine
    _isInitialized = true;
}

void VulkanEngine::cleanup()
{
    if (!_isInitialized)
        return;

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

    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device, imguiPool, nullptr);

    vkDestroySampler(device, _defaultSamplerNearest, nullptr);
    vkDestroySampler(device, _defaultSamplerLinear, nullptr);

    depthImage.Destroy();
    drawImage.Destroy();

    commonSets.Destroy();

    vmaDestroyAllocator(vmaAllocator);

    descriptorAllocator.DestroyPools();
}

void VulkanEngine::draw(const uint32_t frameCount, VkCommandBuffer cmd, Image& targetImage)
{
    mainCamera.update();

    glm::mat4 view = mainCamera.getViewMatrix();

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

    VkExtent3D drawImageExtent = drawImage.imageExtent;

    const VkRenderingInfo renderInfo = vkinit::rendering_info({drawImageExtent.width, drawImageExtent.height}, &colorAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo); {
        //set dynamic viewport and scissor
        VkViewport viewport = {};
        viewport.width = static_cast<float>(drawImageExtent.width);
        viewport.height = static_cast<float>(drawImageExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = drawImageExtent.width;
        scissor.extent.height = drawImageExtent.height;

        vkCmdSetScissor(cmd, 0, 1, &scissor);

        //allocate a new uniform buffer for the scene data
        sceneDataBuffer = Buffer<GPUSceneData>::Allocate(vmaAllocator, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        sceneDataBuffer.Write(&sceneData);

        //add it to the deletion queue of this frame so it gets deleted once its been used

        //create a descriptor set that binds that buffer and update it
        gpuSceneDescriptorSet.AllocateSet(frameDescriptors, commonSets.GPUSceneData);
        gpuSceneDescriptorSet.StageBuffer(shader_layouts::global::SceneData_binding, sceneDataBuffer);
        gpuSceneDescriptorSet.PerformWrites();

        passManager.Draw(cmd, ~0u);
    }
    vkCmdEndRendering(cmd);

    // execute a copy from the draw image into the swapchain
    drawImage.BlitCopyTo(cmd, targetImage);

    // set swapchain image layout to Attachment Optimal so we can draw it
    targetImage.Transition(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //draw imgui into the swapchain image
    const VkRenderingAttachmentInfo targetAttachment = vkinit::attachment_info(targetImage, nullptr);
    const VkRenderingInfo targetRenderInfo = vkinit::rendering_info(windowSize, &targetAttachment, nullptr);

    vkCmdBeginRendering(cmd, &targetRenderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);

    // set swapchain image layout to Present so we can draw it
    targetImage.Transition(cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void VulkanEngine::init_imgui(const DeviceData& deviceData)
{
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    const VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    auto pool_info = vkinit::New<VkDescriptorPoolCreateInfo>(); {
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
        pool_info.pPoolSizes = pool_sizes;
    }

    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    // this initializes imgui for SDL
    ImGui_ImplSDL2_InitForVulkan(deviceData.window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = deviceData.instance;
    init_info.PhysicalDevice = deviceData.physicalDevice;
    init_info.Device = device;
    init_info.Queue = deviceData.graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    //dynamic rendering parameters for imgui to use
    init_info.PipelineRenderingCreateInfo = vkinit::New<VkPipelineRenderingCreateInfo>(); {
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &imageFormat;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    }

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();
}

void VulkanEngine::init_swapchain(const DeviceData& deviceData)
{
    device = deviceData.device;

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
}

void VulkanEngine::init_descriptors()
{
    descriptorAllocator.Init(device);

    commonSets.Init(device);

    //make sure both the descriptor allocator and the new layout get cleaned up properly

    //> frame_desc
    for (auto& [frameDescriptors, sceneDataBuffer] : frameData)
        frameDescriptors.Init(device);
    //< frame_desc
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

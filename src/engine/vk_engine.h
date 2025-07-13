#pragma once

#include <vk_types.h>
#include <ranges>
#include "vk_mem_alloc.h"

#include "camera.h"
#include "vk_descriptors.h"
#include "rendering/CommonSetLayouts.h"
#include "rendering/CommonTextures.h"
#include "rendering/passes/PassDirectory.h"
#include "rendering/Image.h"
#include "rendering/Material.h"
#include "rendering/PassManager.h"
#include "rendering/Swapchain.h"

struct FrameData
{
    DescriptorAllocator frameDescriptors;
    rendering::Buffer<GPUSceneData> sceneDataBuffer;
};

class VulkanEngine
{
public:
    ecs::EntityManager entityManager;

    FrameData frameData[rendering::FRAME_OVERLAP];

    VmaAllocator vmaAllocator;
    VkDevice device;

    Camera mainCamera;

    DescriptorAllocator descriptorAllocator;

    rendering::CommonSetLayouts commonSets;
    rendering::CommonTextures commonTextures;

    DescriptorWriter gpuSceneDescriptorSet;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;

    rendering::Image drawImage;
    rendering::Image depthImage;

    rendering::PassManager passManager;

    GPUSceneData sceneData;

    rendering::Material<default_pass::Pass> defaultMaterial;

    void Init(rendering::SwapchainRenderer* swapchainRenderer);

    void Draw(uint32_t frameCount, VkCommandBuffer cmd, rendering::Image& targetImage);

    void Destroy();

    rendering::Mesh UploadMesh(const rendering::SwapchainRenderer* swapchainRenderer, std::span<uint32_t> indices, std::span<Vertex> vertices) const;
};

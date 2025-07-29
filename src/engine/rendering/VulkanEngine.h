#pragma once

#include "Camera.h"
#include "ecs/EngineAliases.h"
#include "ecs/EntityManager.h"
#include "rendering/CommonSetLayouts.h"
#include "rendering/CommonTextures.h"
#include "pass-system/Material.h"

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

struct FrameData
{
    DescriptorAllocator frameDescriptors;
    rendering::Buffer<GPUSceneData> sceneDataBuffer;
};

// TODO - still needs a lot of clean up
class VulkanEngine
{
    rendering::EngineResources* swapchainRenderer = nullptr;

public:
    FrameData frameData[2];

    Camera mainCamera;

    rendering::CommonSetLayouts commonSets;
    rendering::CommonTextures commonTextures;

    DescriptorWriter gpuSceneDescriptorSet;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;

    rendering::Image drawImage;
    rendering::Image depthImage;

    ecs::MainEntityManager ecsMain;
    ecs::PassEntityManager ecsPass;
    rendering::PassMeshManager passMeshManager;

    GPUSceneData sceneData;

    rendering::Material<default_pass::Pass> defaultMaterial;

    // Easy way to get resources the rendering engine is using. Resources are implicitly cast to pass whatever is needed to a method
    rendering::ResourceHandles& Resource() const;

    void Init(rendering::EngineResources* swapchainRenderer);

    void Draw(uint32_t frameCount, VkCommandBuffer cmd, rendering::Image& targetImage);

    void Destroy();
};

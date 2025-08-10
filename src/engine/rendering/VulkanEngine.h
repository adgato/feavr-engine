#pragma once

#include "Camera.h"
#include "rendering/CommonSetLayouts.h"
#include "rendering/CommonTextures.h"
#include "pass-system/Material.h"

class Core;

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
public:
    rendering::EngineResources* engineResources = nullptr;
    FrameData frameData[2];

    Camera mainCamera;

    rendering::CommonSetLayouts commonSets;
    rendering::CommonTextures commonTextures;

    DescriptorWriter gpuSceneDescriptorSet;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;

    rendering::Image drawImage;
    rendering::Image depthImage;

    ecs::Engine ecsEngine;
    rendering::PassMeshManager passManager;

    GPUSceneData sceneData;

    rendering::Material<default_pass::Pass> defaultMaterial { ecsEngine, passManager };

    // Easy way to get resources the rendering engine is using. Resources are implicitly cast to pass whatever is needed to a method
    rendering::ResourceHandles& Resource() const;

    void Init(rendering::EngineResources* swapchainRenderer);

    void Draw(uint32_t frameCount, VkCommandBuffer cmd, rendering::Image& targetImage);

    // TODO - this method should be moved out of this class eventually
    void SaveScene(const char* filePath);
    void LoadScene(assets_system::AssetID other);

    void Destroy();
};

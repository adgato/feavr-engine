#pragma once

#include "Camera.h"
#include "rendering/CommonSetLayouts.h"
#include "rendering/CommonTextures.h"
#include "pass-system/Material.h"

class Core;


struct FrameData
{
    DescriptorAllocator frameAllocator;
    rendering::Buffer<rendering::GlobalSceneData> sceneDataBuffer;
};

// TODO - still needs a lot of clean up
class RenderingEngine
{
public:
    rendering::RenderingResources& resources;
    rendering::ResourceHandles& resource;
    FrameData frameData[2] {};

    Camera mainCamera {};

    rendering::CommonSetLayouts commonSets {};
    rendering::CommonTextures commonTextures {};

    DescriptorWriter gpuSceneDescriptorSet {};

    VkSampler defaultSamplerLinear = nullptr;
    VkSampler defaultSamplerNearest = nullptr;

    rendering::Image drawImage {};
    rendering::Image depthImage {};

    rendering::GlobalSceneData sceneData {};
    rendering::PassSystem passManager;

    rendering::Material<IdentifyPass> defaultMaterial;

    // Easy way to get resources the rendering engine is using. Resources are implicitly cast to pass whatever is needed to a method

    RenderingEngine(ecs::Engine& engine, rendering::RenderingResources& resources);

    void Init(rendering::RenderingResources& resources);

    void Draw(uint32_t frameCount, VkCommandBuffer cmd, rendering::Image& targetImage);

    void Destroy();
};

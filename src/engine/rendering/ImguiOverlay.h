#pragma once
#include "resources/EngineResources.h"

class ImguiOverlay
{
    VkDevice device = nullptr;
    VkDescriptorPool imguiPool = nullptr;

public:
    void Init(const rendering::ResourceHandles& deviceData);
    void Draw(VkCommandBuffer cmd, rendering::Image& targetImage);
    void Destroy();
};

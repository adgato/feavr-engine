#pragma once
#include "rendering/Swapchain.h"

class ImguiOverlay
{
    VkDevice device = nullptr;
    VkDescriptorPool imguiPool = nullptr;

public:
    void Init(const rendering::DeviceData& deviceData);
    void Draw(VkCommandBuffer cmd, rendering::Image& targetImage);
    void Destroy();
};

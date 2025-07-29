#pragma once
#include "ImguiOverlay.h"
#include "VulkanEngine.h"

class Core
{
public:
    VulkanEngine engine;
    ImguiOverlay imguiOverlay;
    rendering::EngineResources swapchain;

    bool skipDrawing = false;
    
    void Init();
    bool Next();
    void Destroy();
};

#pragma once
#include "ImguiOverlay.h"
#include "utility/ClickOnMeshTool.h"
#include "VulkanEngine.h"

class Core
{
public:
    VulkanEngine engine;
    ImguiOverlay imguiOverlay;
    rendering::EngineResources swapchain;
    rendering::ClickOnMeshTool clickOnMeshTool { swapchain, engine.passManager };

    bool skipDrawing = false;
    
    void Init();
    bool Next();
    void Destroy();
};

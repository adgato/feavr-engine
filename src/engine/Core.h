#pragma once
#include "ImguiOverlay.h"
#include "vk_engine.h"
#include "ecs/Engine.h"

class Core
{
public:
    ecs::EntityManager manager;
    VulkanEngine engine;
    ImguiOverlay imguiOverlay;
    rendering::SwapchainRenderer swapchain;

    bool skipDrawing = false;
    
    void Init();
    bool Next();
    void Destroy();
};

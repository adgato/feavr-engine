#pragma once
#include "vk_engine.h"
#include "ecs/Engine.h"

class Core
{
public:
    ecs::EntityManager manager;
    VulkanEngine engine;
    rendering::SwapchainRenderer swapchain;

    bool skipDrawing = false;
    
    void Init();
    bool Next();
    void Destroy();
};

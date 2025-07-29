#pragma once
#include "rendering/resources/Image.h"

class VulkanEngine;

namespace rendering
{
    class CommonTextures
    {
    public:
        Image errorCheckerboard; 
        
        void Init(EngineResources* swapchainRenderer);
        void Destroy();
    };    
}

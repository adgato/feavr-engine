#pragma once
#include "Image.h"

class VulkanEngine;

namespace rendering
{
    class CommonTextures
    {
    public:
        Image errorCheckerboard; 
        
        void Init(VulkanEngine* engine, SwapchainRenderer* swapchainRenderer);
        void Destroy();
    };    
}

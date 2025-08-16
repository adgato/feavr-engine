#pragma once
#include "rendering/resources/Image.h"

class RenderingEngine;

namespace rendering
{
    class CommonTextures
    {
    public:
        Image errorCheckerboard; 
        
        void Init(RenderingResources* swapchainRenderer);
        void Destroy();
    };    
}

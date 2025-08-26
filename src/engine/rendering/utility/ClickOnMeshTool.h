#pragma once

#include "glm/fwd.hpp"
#include "rendering/resources/RenderingResources.h"
#include "rendering/resources/Image.h"

class Camera;
struct VkCommandBuffer_T;
typedef VkCommandBuffer_T* VkCommandBuffer;

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

namespace rendering
{
    class PassSystem;

    class ClickOnMeshTool
    {
        RenderingResources& resources;
        PassSystem& passManager;
        uint32_t selectFrame = 0;
        Image drawImage {};
        Image depthImage {};
        Buffer<uint32_t> resultBuffer;
        bool waitingSample = false;

    public:
        ClickOnMeshTool(RenderingResources& engineResources, PassSystem& passManager);

        void Init();

        void DrawMeshIndices(VkCommandBuffer cmd, const VkExtent3D& imageExtent, const Camera& camera, glm::vec2 coord);

        bool DrawWaitingSample(uint32_t sufficientFrameDelta = 1) const;

        // call after select mesh command buffer has finished executing.
        uint32_t SampleCoordinate();

        void Destroy();
    };
}

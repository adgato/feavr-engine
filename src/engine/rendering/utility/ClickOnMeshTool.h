#pragma once

#include "hlsl++/vector_int_type.h"
#include "rendering/resources/RenderingResources.h"
#include "rendering/resources/Image.h"

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
        hlslpp::int2 coord {};
        bool waitingSample = false;

    public:
        ClickOnMeshTool(RenderingResources& engineResources, PassSystem& passManager);

        void Init();

        void DrawMeshIndices(VkCommandBuffer cmd, const glm::mat4& cameraView, const hlslpp::int2& coord);

        bool DrawWaitingSample(uint32_t sufficientFrameDelta = 1) const;

        // call after select mesh command buffer has finished executing.
        uint32_t SampleCoordinate();

        void Destroy();
    };
}

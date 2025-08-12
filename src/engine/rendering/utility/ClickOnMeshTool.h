#pragma once

#include "hlsl++/vector_int_type.h"
#include "rendering/resources/EngineResources.h"
#include "rendering/resources/Image.h"

struct VkCommandBuffer_T;
typedef VkCommandBuffer_T* VkCommandBuffer;

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

namespace rendering
{
    class PassMeshManager;

    class ClickOnMeshTool
    {
        EngineResources& resources;
        PassMeshManager& passManager;
        uint32_t selectFrame = 0;
        Image drawImage {};
        Image depthImage {};

    public:
        ClickOnMeshTool(EngineResources& engineResources, PassMeshManager& passManager);

        void Init(const Image& drawTemplate, const Image& depthTemplate);

        void SelectMesh(VkCommandBuffer cmd);

        bool Destroyed() const;
        bool SelectMeshCompleted(uint32_t sufficientFrameDelta = 1) const;

        // call after select mesh command buffer has finished executing.
        uint32_t SampleCoordinate(const hlslpp::int2& coord);

        void Destroy();
    };
}

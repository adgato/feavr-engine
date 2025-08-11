#pragma once

namespace rendering {
    class Image;
    class EngineResources;

    class PassMeshManager;
}

struct VkViewport;

namespace rendering
{
    void SelectMesh(EngineResources& resources, PassMeshManager& passManager, const Image& drawTemplate, const Image& depthTemplate);
}

#pragma once
#include "DefaultPass.h"
#include "Mesh.h"
#include "PassDirectory.h"
#include "SubMesh.h"
#include "ecs/ComponentTypeInfo.h"

class VulkanEngine;

namespace rendering
{
    template <typename T>
    concept ManagedPass = ecs::one_of_v<T, default_pass::Pass>;

    class PassMeshManager
    {
        std::vector<Mesh> meshes {};
        std::vector<int32_t> meshRefCounts {};
        std::size_t nextFreeMesh = 0;
    public:

        default_pass::Pass defaultPass {};

        template <ManagedPass T>
        T& GetPass();

        void Init(VulkanEngine* e);

        void Draw(VkCommandBuffer cmd);

        uint32_t AddMesh(Mesh&& mesh);
        SubMesh ReferenceMesh(uint32_t meshIndex, uint32_t firstIndex = 0, uint32_t indexCount = ~0u);
        void DereferenceMesh(uint32_t meshIndex);

        void Destroy();
    };
}

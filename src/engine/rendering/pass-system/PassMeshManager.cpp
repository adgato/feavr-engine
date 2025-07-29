#include "PassMeshManager.h"
#include "Mesh.h"

namespace rendering
{
    template <ManagedPass T>
    T& PassMeshManager::GetPass()
    {
        if constexpr (std::is_same_v<T, default_pass::Pass>)
        {
            return defaultPass;
        } else
        {
            static_assert(false);
            return {};
        }
    }

    void PassMeshManager::Init(VulkanEngine* e)
    {
        defaultPass.Init(e);
    }

    void PassMeshManager::Draw(const VkCommandBuffer cmd)
    {
        defaultPass.Draw(cmd, meshes);
    }

    uint32_t PassMeshManager::AddMesh(Mesh&& mesh)
    {
        for (; nextFreeMesh < meshes.size(); ++nextFreeMesh)
            if (meshRefCounts[nextFreeMesh] <= 0)
            {
                meshRefCounts[nextFreeMesh] = 0;
                meshes[nextFreeMesh] = std::move(mesh);
                return nextFreeMesh++;
            }
        meshes.emplace_back(std::move(mesh));
        meshRefCounts.emplace_back();
        return nextFreeMesh++;
    }

    SubMesh PassMeshManager::ReferenceMesh(const uint32_t meshIndex, const uint32_t firstIndex /* = 0 */, const uint32_t indexCount /* = ~0u */)
    {
        assert(meshRefCounts[meshIndex] >= 0);
        ++meshRefCounts[meshIndex];

        SubMesh subMesh;
        *subMesh.meshIndex = meshIndex;
        *subMesh.firstIndex = firstIndex;
        *subMesh.indexCount = std::min(indexCount, meshes[meshIndex].indexBuffer.count - firstIndex);
        subMesh.passMeshManager = this;
        return subMesh;
    }

    void PassMeshManager::DereferenceMesh(const uint32_t meshIndex)
    {
        if (meshRefCounts.size() <= meshIndex)
            return;

        assert(meshRefCounts[meshIndex] >= 0);
        if (--meshRefCounts[meshIndex] <= 0)
        {
            meshes[meshIndex].Destroy();
            if (meshIndex < nextFreeMesh)
                nextFreeMesh = meshIndex;
        }
    }

    void PassMeshManager::Destroy()
    {
        for (Mesh& mesh : meshes)
            mesh.Destroy();

        meshes.clear();
        meshRefCounts.clear();
        nextFreeMesh = 0;

        defaultPass.Destroy();
    }

    template default_pass::Pass& PassMeshManager::GetPass();
}

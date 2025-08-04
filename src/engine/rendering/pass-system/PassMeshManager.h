#pragma once
#include <variant>

#include "DefaultPass.h"
#include "Mesh.h"
#include "PassDirectory.h"
#include "SubMesh.h"
#include "assets-system/AssetFile.h"
#include "ecs/EngineAliases.h"
#include "ecs/SingletonEntity.h"

// convenience of management
#define PASSES default_pass::Pass

class VulkanEngine;

namespace rendering
{
    template <typename T>
    concept ManagedPass = ecs::one_of_v<T, PASSES>;

    struct MeshAssetSource
    {
        assets_system::AssetID fromAsset;
        size_t assetMeshIndex;

        bool operator<(const MeshAssetSource& other) const
        {
            if (fromAsset.id < other.fromAsset.id)
                return true;
            if (fromAsset.id > other.fromAsset.id)
                return false;

            if (fromAsset.idx < other.fromAsset.idx)
                return true;
            if (fromAsset.idx > other.fromAsset.idx)
                return false;

            return assetMeshIndex < other.assetMeshIndex;
        }
    };

    struct MeshDirectSource
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    class PassMeshManager
    {
        std::vector<Mesh> meshes {};
        std::vector<std::variant<MeshAssetSource, std::shared_ptr<MeshDirectSource>>> metadata {};
        std::vector<int32_t> refCounts;
        std::size_t nextFreeMesh = 0;

        EngineResources* resources = nullptr;

    public:
        struct
        {
            serial::fsize meshesStart;
            serial::fsize meshesSizeBytes;
        } serializeInfo;

        ecs::SingletonEntity<PASSES> passes = SINGLETON_ENTITY(default_pass::Pass);

        void Init(VulkanEngine* e);

        void Draw(VkCommandBuffer cmd);

        void ReadMeshes(serial::Stream& m);

        void WriteMeshes(serial::Stream& m);

        void Serialize(serial::Stream& m);

        uint32_t AddMesh(Mesh&& mesh, MeshAssetSource&& metaData);

        uint32_t AddMesh(Mesh&& mesh, MeshDirectSource&& metaData);

        SubMesh ReferenceMesh(uint32_t meshIndex, uint32_t firstIndex = 0, uint32_t indexCount = ~0u);

        void DereferenceMesh(uint32_t meshIndex);

        void ReplaceInvalidAssetSources(assets_system::AssetID sourceAsset);
        void FixupReferences(ecs::PassEntityManager& passECS);

        void Destroy();
    };
}

#undef PASSES

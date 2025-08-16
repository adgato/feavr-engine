#pragma once
#include <variant>

#include "DefaultPass.h"
#include "Mesh.h"
#include "PassDirectory.h"
#include "IdentifyPass.h"
#include "StencilOutlinePass.h"
#include "SubMesh.h"
#include "assets-system/AssetFile.h"
#include "assets-system/AssetID.h"
#include "ecs/Engine.h"

class RenderingEngine;

namespace rendering
{
    template <typename T>
    concept ManagedPass = ecs::one_of_v<T, default_pass::Pass, identify_pass::Pass, stencil_outline_pass::Pass>;

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

    class PassSystem
    {
        std::vector<Mesh> meshes {};
        std::vector<std::variant<MeshAssetSource, std::shared_ptr<MeshDirectSource>>> metadata {};
        std::vector<int32_t> refCounts;
        std::size_t nextFreeMesh = 0;

        RenderingResources& resources;

        default_pass::Pass defaultPass;
        identify_pass::Pass identifyPass;
        stencil_outline_pass::Pass stencilOutlinePass;

    public:
        struct
        {
            serial::fsize meshesStart;
            serial::fsize meshesSizeBytes;
        } serializeInfo {};

        PassSystem(RenderingResources& resources, RenderingEngine& renderer, ecs::Engine& engine)
            : resources(resources), defaultPass(renderer, engine), identifyPass(renderer, engine), stencilOutlinePass(renderer, engine) {}

        template <ManagedPass T>
        T& GetPass()
        {
            if constexpr (std::is_same_v<default_pass::Pass, T>)
                return defaultPass;
            if constexpr (std::is_same_v<identify_pass::Pass, T>)
                return identifyPass;
            if constexpr (std::is_same_v<stencil_outline_pass::Pass, T>)
                return stencilOutlinePass;
            throw std::exception();
        }

        std::span<Mesh> GetMeshes();

        void Init();

        void Draw(VkCommandBuffer cmd);

        void ReadMeshes(serial::Stream& m);

        void WriteMeshes(serial::Stream& m);

        void Serialize(serial::Stream& m);

        uint32_t AddMesh(Mesh&& mesh, MeshAssetSource&& metaData);

        uint32_t AddMesh(Mesh&& mesh, MeshDirectSource&& metaData);

        SubMesh ReferenceMesh(uint32_t meshIndex, uint32_t firstIndex = 0, uint32_t indexCount = ~0u);

        void DereferenceMesh(uint32_t meshIndex);

        void ReplaceInvalidAssetSources(assets_system::AssetID sourceAsset);

        void FixupReferences(const ecs::Engine& passECS);

        void Destroy();
    };
}

#include "PassMeshManager.h"
#include "Mesh.h"
#include "serialisation/array.h"
#include <map>

#include "assets-system/AssetManager.h"
#include "ecs/EngineView.h"
#include "rendering/VulkanEngine.h"
#include "rendering/resources/EngineResources.h"

namespace rendering
{
    struct MeshAssetSourceIndex
    {
        serial::fsize vertexOffset;
        serial::fsize indexOffset;
        serial::fsize vertexCount;
        serial::fsize indexCount;
    };

    void PassMeshManager::Init(VulkanEngine* e)
    {
        resources = e->engineResources;
        defaultPass.Init(e);
    }

    void PassMeshManager::Draw(const VkCommandBuffer cmd)
    {
        defaultPass.Draw(cmd, meshes);
    }

    void MeshAssetsToDirectSources(std::vector<std::shared_ptr<MeshDirectSource>>& directSources, const assets_system::AssetID& sourceFileID,
                                   const std::map<MeshAssetSource, size_t>::iterator& it, std::map<MeshAssetSource, size_t>::iterator& jt)
    {
        assets_system::AssetFile sourceFile = assets_system::AssetManager::LoadAsset(sourceFileID);

        if (!sourceFile.HasFormat("SCNE", 0))
        {
            for (; jt != it; ++jt)
                directSources[jt->second] = std::make_shared<MeshDirectSource>();
            return;
        }

        serial::Stream mSource = sourceFile.ReadFromBlob((uint64_t)sourceFile.header["Meshes Start"], (uint64_t)sourceFile.header["Meshes Size"]);

        const size_t sourceMeshCount = mSource.reader.Read<serial::fsize>();

        for (; jt != it; ++jt)
        {
            const size_t index = jt->first.assetMeshIndex;
            if (index >= sourceMeshCount)
            {
                directSources[jt->second] = std::make_shared<MeshDirectSource>();
                continue;
            }

            mSource.reader.JumpAbs(sizeof(serial::fsize) + sizeof(MeshAssetSourceIndex) * index);
            const auto [vertexOffset, indexOffset, vertexCount, indexCount] = mSource.reader.Read<MeshAssetSourceIndex>();

            std::vector<Vertex> vertices(vertexCount);
            std::vector<uint32_t> indices(indexCount);

            mSource.reader.JumpAbs(vertexOffset);
            mSource.reader.ReadArray<Vertex>(vertices.data(), vertexCount);

            mSource.reader.JumpAbs(indexOffset);
            mSource.reader.ReadArray<uint32_t>(indices.data(), indexCount);

            directSources[jt->second] = std::make_shared<MeshDirectSource>(std::move(vertices), std::move(indices));
        }
    }

    void PassMeshManager::ReadMeshes(serial::Stream& m)
    {
        const size_t meshesStart = m.reader.GetCount();
        const size_t sourceMeshCount = m.reader.Read<serial::fsize>();

        assert(meshes.size() == metadata.size());

        meshes.reserve(meshes.size() + sourceMeshCount);
        metadata.reserve(meshes.size() + sourceMeshCount);

        refCounts.resize(meshes.size() + sourceMeshCount);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (size_t index = 0; index < sourceMeshCount; ++index)
        {
            m.reader.JumpAbs(meshesStart + sizeof(serial::fsize) + sizeof(MeshAssetSourceIndex) * index);
            const auto [vertexOffset, indexOffset, vertexCount, indexCount] = m.reader.Read<MeshAssetSourceIndex>();

            vertices.resize(vertexCount);
            indices.resize(indexCount);

            m.reader.JumpAbs(meshesStart + vertexOffset);
            m.reader.ReadArray<Vertex>(vertices.data(), vertexCount);

            m.reader.JumpAbs(meshesStart + indexOffset);
            m.reader.ReadArray<uint32_t>(indices.data(), indexCount);

            meshes.emplace_back(resources->UploadMesh(indices, vertices));
            metadata.emplace_back(MeshAssetSource { assets_system::AssetID::Invalid(), index });
        }
    }

    void PassMeshManager::WriteMeshes(serial::Stream& m)
    {
        // load all the assets used and copy their mesh data for us to serialize.
        std::map<MeshAssetSource, size_t> assetSources;
        std::vector<std::shared_ptr<MeshDirectSource>> directSources(metadata.size());

        for (size_t i = 0; i < metadata.size(); ++i)
        {
            const auto& variant = metadata[i];
            if (const auto* assetSource = std::get_if<MeshAssetSource>(&variant))
                assetSources.insert_or_assign(*assetSource, i);
            else if (const auto* directSource = std::get_if<std::shared_ptr<MeshDirectSource>>(&variant))
                directSources[i] = *directSource;
        }

        assets_system::AssetID currentFileID = assetSources.size() > 0 ? assetSources.begin()->first.fromAsset : assets_system::AssetID::Invalid();
        assets_system::AssetID nextFileID = currentFileID;

        auto jt = assetSources.begin();

        for (auto it = assetSources.begin(); it != assetSources.end(); ++it)
        {
            nextFileID = it->first.fromAsset;

            if (currentFileID.id == nextFileID.id && currentFileID.idx == nextFileID.idx)
                continue;

            MeshAssetsToDirectSources(directSources, currentFileID, it, jt);

            currentFileID = nextFileID;
        }
        if (assetSources.size() > 0)
            MeshAssetsToDirectSources(directSources, nextFileID, assetSources.end(), jt);

        serializeInfo.meshesStart = m.writer.GetCount();
        m.writer.Write<serial::fsize>(meshes.size());

        std::vector<MeshAssetSourceIndex> sourceIndices(meshes.size());

        const size_t sourceIndicesOffset = m.writer.Reserve<MeshAssetSourceIndex>(sourceIndices.size());

        for (size_t i = 0; i < directSources.size(); ++i)
        {
            const auto& mesh = directSources[i];
            assert(mesh);

            const size_t vertexCount = mesh->vertices.size();
            const size_t indexCount = mesh->indices.size();

            MeshAssetSourceIndex sourceIndex;
            sourceIndex.vertexOffset = m.writer.GetCount() - serializeInfo.meshesStart;
            sourceIndex.vertexCount = vertexCount;

            m.writer.WriteArray<Vertex>(mesh->vertices.data(), vertexCount);

            sourceIndex.indexOffset = m.writer.GetCount() - serializeInfo.meshesStart;
            sourceIndex.indexCount = indexCount;

            m.writer.WriteArray<uint32_t>(mesh->indices.data(), indexCount);

            sourceIndices[i] = sourceIndex;
        }

        m.writer.WriteOverArray<MeshAssetSourceIndex>(sourceIndices.data(), sourceIndices.size(), sourceIndicesOffset);
        serializeInfo.meshesSizeBytes = m.writer.GetCount() - serializeInfo.meshesStart;
    }

    void PassMeshManager::Serialize(serial::Stream& m)
    {
        if (m.reading)
            ReadMeshes(m);
        else
            WriteMeshes(m);

        defaultPass.Serialize(m);
    }

    uint32_t PassMeshManager::AddMesh(Mesh&& mesh, MeshAssetSource&& metaData)
    {
        for (; nextFreeMesh < meshes.size(); ++nextFreeMesh)
            if (refCounts[nextFreeMesh] <= 0)
            {
                refCounts[nextFreeMesh] = 0;
                meshes[nextFreeMesh] = std::move(mesh);
                metadata[nextFreeMesh] = metaData;
                return nextFreeMesh;
            }
        meshes.emplace_back(std::move(mesh));
        refCounts.emplace_back();
        metadata.emplace_back(std::move(metaData));
        return nextFreeMesh++;
    }

    uint32_t PassMeshManager::AddMesh(Mesh&& mesh, MeshDirectSource&& metaData)
    {
        auto metaDataPtr = std::make_shared<MeshDirectSource>(std::move(metaData));

        for (; nextFreeMesh < meshes.size(); ++nextFreeMesh)
            if (refCounts[nextFreeMesh] <= 0)
            {
                refCounts[nextFreeMesh] = 0;
                meshes[nextFreeMesh] = std::move(mesh);
                metadata[nextFreeMesh] = std::move(metaDataPtr);
                return nextFreeMesh;
            }
        meshes.emplace_back(std::move(mesh));
        refCounts.emplace_back();
        metadata.emplace_back(std::move(metaDataPtr));
        return nextFreeMesh++;
    }

    SubMesh PassMeshManager::ReferenceMesh(const uint32_t meshIndex, const uint32_t firstIndex /* = 0 */, const uint32_t indexCount /* = ~0u */)
    {
        assert(refCounts[meshIndex] >= 0);
        ++refCounts[meshIndex];

        const Mesh& mesh = meshes[meshIndex];

        SubMesh subMesh;
        *subMesh.meshIndex = meshIndex;
        *subMesh.firstIndex = firstIndex;
        *subMesh.indexCount = mesh.IsValid() ? std::min(indexCount, mesh.indexBuffer.count - firstIndex) : indexCount;
        subMesh.passMeshManager = this;
        return subMesh;
    }

    void PassMeshManager::DereferenceMesh(const uint32_t meshIndex)
    {
        if (refCounts.size() <= meshIndex)
            return;

        assert(refCounts[meshIndex] >= 0);
        if (--refCounts[meshIndex] <= 0)
        {
            meshes[meshIndex].Destroy();
            if (meshIndex < nextFreeMesh)
                nextFreeMesh = meshIndex;
        }
    }

    void PassMeshManager::ReplaceInvalidAssetSources(const assets_system::AssetID sourceAsset)
    {
        assert(meshes.size() == metadata.size());
        for (auto& variant : metadata)
        {
            if (auto* assetSource = std::get_if<MeshAssetSource>(&variant))
            {
                if (assetSource->fromAsset.IsInvalid())
                    assetSource->fromAsset = sourceAsset;
            }
        }
    }

    void PassMeshManager::FixupReferences(const ecs::Engine& passECS)
    {
        refCounts.clear();
        refCounts.resize(meshes.size());

        ecs::EngineView<SubMesh> view(passECS);
        for (auto [_, submesh] : view)
        {
            submesh.passMeshManager = this;
            ++refCounts[submesh.meshIndex];
        }

        nextFreeMesh = meshes.size();

        for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
            if (refCounts[meshIndex] <= 0)
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
        metadata.clear();
        refCounts.clear();
        nextFreeMesh = 0;

        defaultPass.Destroy();
    }
}

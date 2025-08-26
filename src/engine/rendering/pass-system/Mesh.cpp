#include "Mesh.h"

#include <lz4.h>

#include "glm/common.hpp"
#include "rendering/resources/RenderingResources.h"

using namespace rendering;

using SerialVertex = serial::Serializable<0, serial::array<Vertex>>;
using SerialIndex = serial::Serializable<1, serial::array<uint32_t>>;

void Mesh::Compress(const std::span<std::byte> data)
{
    const int bound = LZ4_compressBound(data.size());
    compressedMeshData.Resize(bound);

    const int size = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(compressedMeshData.data()),
        data.size(),
        compressedMeshData.size()
    );

    compressedMeshData.Resize(size);
    uncompressedSize = data.size();
}

void Mesh::Uncompress(serial::array<std::byte>& data)
{
    data.Resize(uncompressedSize);

    const int writtenSize = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressedMeshData.data()),
        reinterpret_cast<char*>(data.data()),
        compressedMeshData.size(),
        data.size()
    );
    assert(writtenSize > 0);
}

void Mesh::CalculateBounds(const serial::array<Vertex> vertices)
{
    boundsMin = { FLT_MAX, FLT_MAX, FLT_MAX };
    boundsMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const glm::vec3& pos = vertices.data()[i].position;
        boundsMin = glm::min(boundsMin, pos);
        boundsMax = glm::max(boundsMax, pos);
    }
}

void Mesh::SetMeshData(const serial::array<uint32_t> indices, const serial::array<Vertex> vertices)
{
    serial::Stream meshDataParser {};
    meshDataParser.InitWrite();

    SerialVertex serialVertex { vertices };
    SerialIndex serialIndex { indices };
    meshDataParser.SerializeComponent(serialVertex, serialIndex);

    Compress(meshDataParser.writer.AsSpan());
}

void Mesh::UploadMesh(const RenderingResources& resources)
{
    serial::array<std::byte> uncompressedData {};
    Uncompress(uncompressedData);
    serial::Stream meshDataParser {};
    meshDataParser.InitRead();
    meshDataParser.reader.ViewFrom({ uncompressedData.data(), uncompressedData.size() });

    SerialVertex vertices {};
    SerialIndex indices {};
    meshDataParser.SerializeComponent(vertices, indices);

    CalculateBounds(vertices);

    const size_t vertexBufferSize = vertices->size() * sizeof(Vertex);
    const size_t indexBufferSize = indices->size() * sizeof(uint32_t);

    //create vertex buffer
    vertexBuffer = Buffer<Vertex>::Allocate(resources.resource, vertices->size(),
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, HostAccess::NONE, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    //find the adress of the vertex buffer
    const VkBufferDeviceAddressInfo deviceAdressInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = vertexBuffer.buffer };
    vertexBufferAddress = vkGetBufferDeviceAddress(resources.resource, &deviceAdressInfo);

    //create index buffer
    indexBuffer = Buffer<uint32_t>::Allocate(resources.resource, indices->size(),
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             HostAccess::NONE, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    Buffer<std::byte> staging = Buffer<std::byte>::Allocate(resources.resource, vertexBufferSize + indexBufferSize,
                                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HostAccess::SEQUENTIAL_WRITE);

    std::memcpy(staging.Access(), vertices->data(), vertexBufferSize);
    std::memcpy(staging.Access() + vertexBufferSize, indices->data(), indexBufferSize);

    resources.ImmediateSumbit([&](const VkCommandBuffer cmd)
    {
        VkBufferCopy vertexCopy;
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy;
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, indexBuffer.buffer, 1, &indexCopy);
    });

    staging.Destroy();
    uncompressedData.Destroy();
}

void Mesh::Serialize(serial::Stream& m)
{
    serial::array<std::byte> uncompressedData {};
    if (m.reading)
    {
        uncompressedData.Serialize(m);
        Compress({ uncompressedData.data(), uncompressedData.size() });
    } else
    {
        Uncompress(uncompressedData);
        uncompressedData.Serialize(m);
    }
    uncompressedData.Destroy();
}

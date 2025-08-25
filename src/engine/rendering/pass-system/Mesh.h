#pragma once
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "rendering/resources/Buffer.h"
#include "serialisation/array.h"

namespace rendering
{
    class RenderingResources;

    struct Vertex
    {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };
}

struct Mesh
{
    rendering::Buffer<rendering::Vertex> vertexBuffer;
    rendering::Buffer<uint32_t> indexBuffer;
    VkDeviceAddress vertexBufferAddress;

private:
    serial::array<std::byte> compressedMeshData {};
    size_t uncompressedSize = 0;

    void Compress(std::span<std::byte> data);

    void Uncompress(serial::array<std::byte>& data);

public:
    void SetMeshData(serial::array<uint32_t> indices, serial::array<rendering::Vertex> vertices);

    void UploadMesh(const rendering::RenderingResources& resources);

    void Serialize(serial::Stream& m);

    bool IsValid() const
    {
        return vertexBuffer.buffer && indexBuffer.buffer && vertexBufferAddress;
    }

    void Destroy()
    {
        vertexBuffer.Destroy();
        indexBuffer.Destroy();
        compressedMeshData.Destroy();
    }
};

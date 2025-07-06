#include "SubMesh.h"

namespace rendering
{
    Mesh::Mesh(Mesh&& other) noexcept
    {
        indexBuffer = other.indexBuffer;
        vertexBuffer = other.vertexBuffer;

        vertexBufferAddress = other.vertexBufferAddress;

        other.indexBuffer.buffer = VK_NULL_HANDLE;
        other.vertexBuffer.buffer = VK_NULL_HANDLE;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        Destroy();
        indexBuffer = other.indexBuffer;
        vertexBuffer = other.vertexBuffer;

        vertexBufferAddress = other.vertexBufferAddress;

        other.indexBuffer.buffer = VK_NULL_HANDLE;
        other.vertexBuffer.buffer = VK_NULL_HANDLE;

        return *this;
    }

    Mesh::~Mesh()
    {
        Destroy();
    }

    void Mesh::Destroy()
    {
        indexBuffer.Destroy();
        vertexBuffer.Destroy();
    }
}

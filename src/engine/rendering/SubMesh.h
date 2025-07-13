#pragma once
#include "Buffer.h"
#include "ecs/Common.h"

namespace rendering
{

    struct Mesh
    {
        VkDeviceAddress vertexBufferAddress;
        
        Buffer<uint32_t> indexBuffer;
        Buffer<Vertex> vertexBuffer;
        
        Mesh() = default;
        Mesh(const Mesh& other) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh();

    private:
        void Destroy();
    };
    
    struct SubMesh
    {
        uint32_t indexCount;
        uint32_t firstIndex;
        
        std::shared_ptr<Mesh> mesh;
        std::vector<ecs::EntityID> entities;
    };
}

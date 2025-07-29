#pragma once
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "rendering/resources/Buffer.h"

namespace rendering
{
    struct Vertex
    {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 color;
    };

    struct Mesh
    {
        Buffer<Vertex> vertexBuffer;
        Buffer<uint32_t> indexBuffer;

        VkDeviceAddress vertexBufferAddress;

        void Destroy()
        {
            vertexBuffer.Destroy();
            indexBuffer.Destroy();
        }
    };
}

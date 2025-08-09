#pragma once
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "rendering/resources/Buffer.h"
#include "serialisation/Stream.h"

namespace rendering
{
    struct Transform
    {
        glm::mat4 transform;

        void Serialize(serial::Stream& m)
        {
            m.SerializeArray<float>(glm::value_ptr(transform), 16);
        }
    };

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

        bool IsValid() const
        {
            return vertexBuffer.buffer && indexBuffer.buffer && vertexBufferAddress;
        }

        void Destroy()
        {
            vertexBuffer.Destroy();
            indexBuffer.Destroy();
        }
    };
}

#pragma once
#include "ecs/EntityRef.h"
#include "serialisation/Stream.h"

namespace ecs
{
    class Engine;
}

namespace rendering
{
    class PassSystem;
}

struct SubMesh
{
    SERIALIZABLE(0, ecs::EntityRef) mesh;
    SERIALIZABLE(1, uint32_t) firstIndex;
    SERIALIZABLE(2, uint32_t) indexCount;
    ecs::Engine* engine;

    void Serialize(serial::Stream& m)
    {
        m.SerializeComponent(mesh, firstIndex, indexCount);
    }

    void ReferenceMesh(ecs::Engine& engine, ecs::Entity mesh, uint32_t firstIndex = 0, uint32_t indexCount = ~0u);

    void Destroy();
};

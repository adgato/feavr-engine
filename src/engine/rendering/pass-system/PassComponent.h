#pragma once
#include "ecs/EntityRef.h"
#include "serialisation/array.h"

struct SubMesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
};

template <typename>
struct PassComponent
{
    SERIALIZABLE(0, ecs::EntityRef) mesh = { ecs::BadMaxEntity };
    SERIALIZABLE(1, serial::array<SubMesh>) submeshes;
    ecs::EntityID prevMesh = ecs::BadMaxEntity;

    void Serialize(serial::Stream& m)
    {
        m.SerializeComponent(mesh, submeshes);
    }

    void UpdateMesh(ecs::Entity newMesh)
    {
        prevMesh = mesh->id;
        mesh->id = newMesh;
        submeshes->Resize(0);
    }

    void Destroy()
    {
        submeshes->Destroy();
    }
};

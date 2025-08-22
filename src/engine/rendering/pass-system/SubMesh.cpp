#include "SubMesh.h"
#include "PassSystem.h"

void SubMesh::ReferenceMesh(ecs::Engine& engine, ecs::Entity mesh, const uint32_t firstIndex /* = 0 */, const uint32_t indexCount /* = ~0u */)
{
    this->engine = &engine;
    this->mesh->id = mesh;
    *this->firstIndex = firstIndex;

    Mesh renderMesh = engine.Get<Mesh>(mesh);
    renderMesh.Reference();
    *this->indexCount = renderMesh.IsValid() ? std::min(indexCount, renderMesh.indexBuffer.count - firstIndex) : indexCount;
}

void SubMesh::Destroy()
{
    assert(engine);
    if (Mesh* renderMesh = engine->TryGet<Mesh>(mesh->id))
    {
        if (renderMesh->Dereference())
            engine->Remove<Mesh>(mesh->id);
    }
}
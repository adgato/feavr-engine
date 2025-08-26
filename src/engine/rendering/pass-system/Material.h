#pragma once
#include "Mesh.h"
#include "rendering/pass-system/PassSystem.h"
#include "rendering/pass-system/PassComponent.h"

namespace rendering
{
    template <typename... Passes>
    class Material
    {
        ecs::Engine& engine;
        PassSystem& passManager;

        template <ManagedPass T> requires (ecs::one_of_v<T, Passes...>)
        PassComponent<T> Default(const std::span<SubMesh> submeshes) const
        {
            PassComponent<T> pass {};
            *pass.submeshes = serial::array<SubMesh>::NewFromData(submeshes.data(), submeshes.size());
            return pass;
        }

    public:
        Material(ecs::Engine& engine, PassSystem& passManager)
            : engine(engine),
              passManager(passManager) {}

        template <ManagedPass T> requires (ecs::one_of_v<T, Passes...>)
        T& Get() const
        {
            return passManager.GetPass<T>();
        }

        void Apply(ecs::Entity e, bool addFullSubmesh) const
        {
            SubMesh submesh;

            if (addFullSubmesh)
            {
                if (const Model* model = engine.TryGet<Model>(e))
                {
                    const Mesh& mesh = engine.Get<Mesh>(model->meshRef->id);
                    const uint32_t fullIndexCount = mesh.IsValid() ? mesh.indexBuffer.count : ~0u;
                    submesh = { 0, fullIndexCount };
                } else
                    addFullSubmesh = false;
            }

            (engine.Add<PassComponent<Passes>>(
                e,
                Default<Passes>(addFullSubmesh ? std::span { &submesh, 1u } : std::span<SubMesh> {})
            ), ...);
        }

        void AddSubmesh(ecs::Entity e, SubMesh submesh) const
        {
            (engine.Get<PassComponent<Passes>>(e).submeshes->push_back(submesh), ...);
        }
    };
}

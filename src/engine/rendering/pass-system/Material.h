#pragma once
#include "rendering/pass-system/PassSystem.h"
#include "rendering/pass-system/PassComponent.h"

namespace rendering
{
    template <typename... Passes>
    class Material
    {
        ecs::Engine& engine;
        PassSystem& passManager;
        std::array<uint32_t, sizeof...(Passes)> passGroups {};

    public:
        Material(ecs::Engine& engine, PassSystem& passManager)
            : engine(engine),
              passManager(passManager) {}

        template <ManagedPass T> requires (ecs::one_of_v<T, Passes...>)
        uint32_t GetPassGroup()
        {
            return passGroups[ecs::index_of_type_v<T, Passes...>];
        }

        template <ManagedPass T> requires (ecs::one_of_v<T, Passes...>)
        void SetPassGroup(const uint32_t group)
        {
            passGroups[ecs::index_of_type_v<T, Passes...>] = group;
        }

        template <ManagedPass T> requires (ecs::one_of_v<T, Passes...>)
        T& Get()
        {
            return passManager.GetPass<T>();
        }

        ecs::Entity AddSubMesh(const ecs::EntityID meshID, const std::span<ecs::EntityRef> entities, const uint32_t firstIndex = 0, const uint32_t indexCount = ~0u)
        {
            ecs::Entity e = engine.New();
            SubMesh submesh {};
            submesh.ReferenceMesh(engine, meshID, firstIndex, indexCount);
            engine.Add<SubMesh>(e, submesh);
            (engine.Add<PassComponent<Passes>>(
                e,
                PassComponent<Passes>
                {
                    .passGroup = { passGroups[ecs::index_of_type_v<Passes, Passes...>] },
                    .transforms = { serial::array<ecs::EntityRef>::NewFromData(entities.data(), entities.size()) }
                }
            ), ...);
            return e;
        }

        void RemoveMesh(ecs::Entity subMeshID)
        {
            (engine.Remove<Passes>(subMeshID), ...);
        }
    };
}

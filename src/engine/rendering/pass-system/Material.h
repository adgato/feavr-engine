#pragma once
#include "ecs/EntityManager.h"
#include "rendering/pass-system/PassMeshManager.h"
#include "rendering/pass-system/PassComponent.h"

namespace rendering
{
    template <ManagedPass... Passes>
    class Material
    {
        PassMeshManager* meshManager = nullptr;
        ecs::PassEntityManager* entityManager = nullptr;
        std::array<uint32_t, sizeof...(Passes)> passGroups {};

    public:
        static Material Init(PassMeshManager* passManager, ecs::PassEntityManager* entityManager)
        {
            Material material;
            material.meshManager = passManager;
            material.entityManager = entityManager;
            return material;
        }

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
            return meshManager->GetPass<T>();
        }

        ecs::Entity AddSubMesh(const uint32_t meshIndex, const uint32_t firstIndex = 0, const uint32_t indexCount = ~0u)
        {
            return entityManager->NewEntity<SubMesh, passes::PassComponent<Passes>...>
            (meshManager->ReferenceMesh(meshIndex, firstIndex, indexCount), passes::PassComponent<Passes>
             {
                 .passGroup = { passGroups[ecs::index_of_type_v<Passes, Passes...>] },
                 .entities = { serial::array<ecs::EntityID>::NewReserve(32) }
             }...);
        }

        void RemoveMesh(ecs::Entity subMeshID)
        {
            entityManager->RemoveComponents<Passes...>(subMeshID);
        }
    };
}

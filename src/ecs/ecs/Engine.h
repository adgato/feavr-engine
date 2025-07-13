#pragma once
#include <bitset>
#include <unordered_set>
#include <ranges>
#include <cstring>

#include "ArchetypeData.h"
#include "SerialTypes.h"
#include "TypeIndexer.h"
#include "UpdateQueue.h"

namespace ecs
{
    class Serial;

    class EntityManager
    {
        friend Serial;

        struct EntityLocation
        {
            uint archetype;
            uint index;
        };

        serial::SerialManager serialManager {};

        std::vector<ArchetypeData> archetypeData = std::vector<ArchetypeData>(1);
        std::vector<signature> archetypeSignatures = std::vector<signature>(1);
        std::unordered_map<signature, uint> archetypeSignaturesInv {};

        std::vector<EntityLocation> entityLocations {};

        std::vector<std::vector<UpdateInstr>> entityUpdateQueue {};
        std::unordered_map<EntityID, signature> updateEntityTypes {};
        std::unordered_set<signature> updateNewSignatures {};
        std::vector<TypeID> updateNewType {};
        EntityID freshCount = 0;


        Entity NewEntity();

        void AddComponent(Entity e, const std::byte* data, const TypeID type)
        {
            assert(e < entityLocations.size());
            entityUpdateQueue[e].push_back(UpdateInstr::NewRawAdd(data, type));
        }

    public:
        template <ComponentType T, typename... Ts> requires (ComponentType<Ts> && ...)
        Entity NewEntity(const T& data, const Ts&... datas)
        {
            Entity e = NewEntity();
            AddComponent<T>(e, data);
            (AddComponents<Ts>(e, datas), ...);
            return e;
        }

        template <ComponentType T>
        T& GetComponent(Entity e)
        {
            assert(HasComponent<T>(e));
            auto [archetype, index] = entityLocations[e];
            return *reinterpret_cast<T*>(archetypeData[archetype].GetElem(index, GetComponentID<T>()));
        }

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        std::tuple<Ts&...> GetComponents(Entity e)
        {
            assert(HasComponents<Ts...>(e));
            auto [archetype, index] = entityLocations[e];
            const ArchetypeData& data = archetypeData[archetype];
            return std::tuple<Ts&...>(*reinterpret_cast<Ts*>(data.GetElem(index, GetComponentID<Ts>()))...);
        }

        template <ComponentType T>
        void AddComponent(Entity e, const T& data)
        {
            assert(e < entityLocations.size());
            entityUpdateQueue[e].push_back(UpdateInstr::NewAdd(data, GetComponentID<T>()));
        }

        template <ComponentType T>
        void RemoveComponent(Entity e)
        {
            assert(e < entityLocations.size());
            entityUpdateQueue[e].push_back(UpdateInstr::NewRemove(GetComponentID<T>()));
        }

        template <ComponentType T>
        bool HasComponent(Entity e) const
        {
            if (e >= entityLocations.size())
                return false;

            const signature& archetype = archetypeSignatures[entityLocations[e].archetype];
            return archetype[GetComponentID<T>()];
        }

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        void AddComponents(Entity e, const Ts&... data) { (AddComponent<Ts>(e, data), ...); }

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        void RemoveComponents(Entity e) { (RemoveComponent<Ts>(e), ...); }

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        bool HasComponents(Entity e) const
        {
            if (e >= entityLocations.size())
                return false;

            const signature& archetype = archetypeSignatures[entityLocations[e].archetype];

            return (archetype[GetComponentID<Ts>()] && ...);
        }

        bool HasAnyComponent(Entity e) const
        {
            return e < entityLocations.size() && archetypeSignatures[entityLocations[e].archetype].any();
        }

        void RemoveAllComponents(Entity e)
        {
            if (e >= entityLocations.size())
                return;

            const signature archetype = archetypeSignatures[entityLocations[e].archetype];
            for (uint type = 0; type < NumTypes; ++type)
            {
                if (archetype[type])
                    entityUpdateQueue[e].push_back(UpdateInstr::NewRemove(type));
            }
        }

        void Destroy();

        void RefreshComponents();

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        class View
        {
            EntityManager* manager;

        public:
            explicit View(EntityManager* manager) : manager(manager) {}

            class iterator
            {
                uint entityIndex = 0;
                uint entityCount = 0;
                uint archIndex = 1;
                EntityManager* manager;

                constexpr signature ViewSignature() noexcept
                {
                    signature viewtype;
                    ((viewtype[GetComponentID<Ts>()] = true), ...);
                    return viewtype;
                }

                void FindNextValidArchetype()
                {
                    const auto& data = manager->archetypeData;
                    const auto& signatures = manager->archetypeSignatures;

                    while (archIndex < data.size())
                    {
                        entityCount = data[archIndex].GetCount();
                        if (entityCount > 0 && (signatures[archIndex] & ViewSignature()) == ViewSignature())
                            return;
                        ++archIndex;
                    }
                    archIndex = data.size();
                }

            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = std::tuple<Entity, Ts&...>;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type*;
                using reference = value_type;

                iterator(EntityManager* manager, const bool isBegin) : manager(manager)
                {
                    if (isBegin)
                        FindNextValidArchetype();
                    else
                        archIndex = static_cast<uint>(manager->archetypeData.size());
                }

                reference operator*() const
                {
                    const auto& data = manager->archetypeData[archIndex];
                    return std::tuple<Entity, Ts&...>(data.entities[entityIndex], *reinterpret_cast<Ts*>(data.GetElem(entityIndex, GetComponentID<Ts>()))...);
                }

                iterator& operator++()
                {
                    if (++entityIndex >= entityCount)
                    {
                        ++archIndex;
                        entityIndex = 0;
                        FindNextValidArchetype();
                    }
                    return *this;
                }

                bool operator==(const iterator& other) const
                {
                    return archIndex == other.archIndex;
                }

                bool operator!=(const iterator& other) const
                {
                    return archIndex != other.archIndex;
                }
            };

            iterator begin() const { return iterator(manager, true); }
            iterator end() const { return iterator(manager, false); }
        };

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        View<Ts...> CreateView() const { return View<Ts...>(this); }
    };
}

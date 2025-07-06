#pragma once
#include <bitset>
#include <unordered_set>
#include <ranges>
#include <functional>

#include "RawArrays.h"
#include "TypeIndexer.h"
#include "UpdateQueue.h"

namespace ecs
{
    class EntityManager
    {
        using signature = std::bitset<NumTypes>;

        struct EntityLocation
        {
            uint archetype;
            uint index;
        };

        std::array<TypeInfo, NumTypes> typeInfo{};
        std::array<std::function<void(void*)>, NumTypes> destroyers{};

        std::vector<ArchetypeData> archetypeData = std::vector<ArchetypeData>(1);
        std::vector<signature> archetypeSignatures = std::vector<signature>(1);
        std::unordered_map<signature, uint> archetypeLocation;

        std::vector<EntityLocation> entityLocations{};

        std::vector<std::vector<UpdateInstr>> entityUpdateQueue{};
        std::unordered_map<EntityID, signature> updateEntityTypes{};
        std::unordered_set<signature> updateNewSignatures{};
        std::vector<uint> updateNewOrder{};
        EntityID freshCount = 0;

    public:
        template <ComponentType T, typename... Ts> requires (ComponentType<Ts> && ...)
        Entity NewEntity(const T& data, const Ts&... datas)
        {
            ArchetypeData& cleanEntities = archetypeData[0];
            EntityID e;
            if (freshCount == cleanEntities.GetCount())
            {
                e = static_cast<EntityID>(entityLocations.size());
                entityLocations.push_back({0, freshCount});
                entityUpdateQueue.emplace_back();
                cleanEntities.IncCount(e);
            }
            else
                e = cleanEntities.entities[cleanEntities.GetCount() - freshCount - 1];

            freshCount++;
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

            constexpr size_t id = GetComponentID<T>();
            if (!destroyers[id])
            {
                typeInfo[id] = {sizeof(T), alignof(T)};
                destroyers[id] = [](void* t) { static_cast<T*>(t)->Destroy(); };
            }

            entityUpdateQueue[e].push_back(UpdateInstr::NewAdd(data, id));
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
            for (uint order = 0; order < NumTypes; ++order)
            {
                if (archetype[order])
                    entityUpdateQueue[e].push_back(UpdateInstr::NewRemove(order));
            }
        }

        void Destroy()
        {
            for (EntityID e = 0; e < entityLocations.size(); ++e)
                RemoveAllComponents(e);
            RefreshComponents();
        }

        void RefreshComponents()
        {
            freshCount = 0;

            // create map from entities to their new archetypes
            for (EntityID entity = 0; entity < entityUpdateQueue.size(); ++entity)
                for (auto& [order, data] : entityUpdateQueue[entity])
                {
                    if (!updateEntityTypes.contains(entity))
                        updateEntityTypes[entity] = archetypeSignatures[entityLocations[entity].archetype];

                    // data == null means remove component
                    updateEntityTypes[entity][order] = static_cast<bool>(data);
                }

            // find new archetypes
            for (const signature& archetype : updateEntityTypes | std::views::values)
                updateNewSignatures.insert(archetype);
            for (const signature& archetype : archetypeSignatures)
                updateNewSignatures.erase(archetype);

            // create new archetype data holders
            for (const signature& archetype : updateNewSignatures)
            {
                updateNewOrder.clear();
                for (uint order = 0; order < NumTypes; ++order)
                {
                    if (archetype[order])
                        updateNewOrder.push_back(order);
                }
                archetypeLocation[archetype] = static_cast<uint>(archetypeData.size());
                archetypeData.emplace_back(updateNewOrder, typeInfo);
                archetypeSignatures.push_back(archetype);
            }
            updateNewSignatures.clear();
            updateNewOrder.clear();

            // move entity to correct container and initialise data
            for (auto [entity, archetype] : updateEntityTypes)
            {
                const uint archetype_index = archetypeLocation[archetype];
                ArchetypeData& data = archetypeData[archetype_index];

                const EntityLocation old_location = entityLocations[entity];
                ArchetypeData& old_data = archetypeData[old_location.archetype];
                const signature& old_archetype = archetypeSignatures[old_location.archetype];

                std::vector<UpdateInstr>& updateQueue = entityUpdateQueue[entity];

                entityLocations[entity] = {archetype_index, static_cast<uint>(data.GetCount())};

                data.IncCount(entity);

                signature updating{};

                // init all added components, destroy all removed components
                while (!updateQueue.empty())
                {
                    const auto& [i, init_bytes] = updateQueue.back();
                    updating[i] = true;

                    if (!archetype[i])
                    {
                        // destroy unused added components
                        if (init_bytes)
                            destroyers[i](init_bytes.get());

                        updateQueue.pop_back();
                        continue;
                    }

                    // this type will be added, init with most recent update data
                    archetype[i] = false;
                    data.SetElem(i, init_bytes.get());

                    updateQueue.pop_back();
                }

                // move or destroy all existing components
                for (uint i = 0; i < NumTypes; ++i)
                    if (old_archetype[i])
                    {
                        std::byte* elem = old_data.GetElem(old_location.index, i);
                        if (updating[i])
                            destroyers[i](elem);
                        else
                            data.SetElem(i, elem);
                    }

                Entity moved = old_data.RemoveElem(old_location.index);
                if (entity != moved)
                    entityLocations[moved].index = old_location.index;
            }
            updateEntityTypes.clear();
        }

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        class View
        {
            EntityManager* manager;

        public:
            explicit View(EntityManager* manager) : manager(manager)
            {
            }

            class iterator
            {
                uint entityIndex = 0;
                uint archIndex = 1;
                EntityManager* manager;

                static constexpr signature ViewSignature() noexcept
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
                        if ((signatures[archIndex] & ViewSignature()) == ViewSignature() && data[archIndex].GetCount() > 0)
                            return;
                        ++archIndex;
                    }
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
                    {
                        FindNextValidArchetype();
                    }
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
                    if (++entityIndex >= manager->archetypeData[archIndex].GetCount())
                    {
                        ++archIndex;
                        entityIndex = 0;
                        FindNextValidArchetype();
                    }
                    return *this;
                }

                bool operator==(const iterator& other) const
                {
                    return entityIndex == other.entityIndex && archIndex == other.archIndex;
                }

                bool operator!=(const iterator& other) const
                {
                    return entityIndex != other.entityIndex || archIndex != other.archIndex;
                }
            };

            iterator begin() const { return iterator(manager, true); }
            iterator end() const { return iterator(manager, false); }
        };

        template <typename... Ts> requires (ComponentType<Ts> && ...)
        View<Ts...> CreateView() const { return View<Ts...>(*this); }
    };
}

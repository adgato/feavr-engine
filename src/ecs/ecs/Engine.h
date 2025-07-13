#pragma once
#include <bitset>
#include <format>
#include <unordered_set>
#include <ranges>
#include <cstring>

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

        SerialManager serialManager {};

        std::vector<ArchetypeData> archetypeData = std::vector<ArchetypeData>(1);
        std::vector<signature> archetypeSignatures = std::vector<signature>(1);
        std::unordered_map<signature, uint> archetypeSignaturesInv {};

        std::vector<EntityLocation> entityLocations {};

        std::vector<std::vector<UpdateInstr>> entityUpdateQueue {};
        std::unordered_map<EntityID, signature> updateEntityTypes {};
        std::unordered_set<signature> updateNewSignatures {};
        std::vector<uint> updateNewOrder {};
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
                entityLocations.push_back({ 0, freshCount });
                entityUpdateQueue.emplace_back();
                cleanEntities.IncCount(e);
            } else
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
            serialManager.Reset();
        }

        void SaveTo(const char* filePath)
        {
            RefreshComponents();

            serialManager.SetModeWrite();

            serialManager.writer.WriteArray<char>(ComponentTypeNames, std::strlen(ComponentTypeNames) + 1);

            serialManager.writer.Write<uint64_t>(entityLocations.size());
            serialManager.writer.WriteArray<EntityLocation>(entityLocations.data(), entityLocations.size());

            std::vector<std::byte> signatures = WriteBitset(archetypeSignatures);
            serialManager.writer.Write<uint64_t>(archetypeData.size());
            serialManager.writer.WriteArray<std::byte>(signatures.data(), signatures.size());
            signatures.clear();

            for (uint k = 0; k < archetypeData.size(); ++k)
            {
                ArchetypeData& data = archetypeData[k];
                signature archetype = archetypeSignatures[k];

                const size_t count = data.GetCount();
                serialManager.writer.Write<uint64_t>(count);
                serialManager.writer.WriteArray<EntityID>(data.entities.data(), count);
                for (size_t type = 0; type < NumTypes; ++type)
                {
                    if (!archetype[type])
                        continue;
                    for (size_t i = 0; i < count; ++i)
                    {
                        void* elem = data.GetElem(i, type);
                        ComponentInfo.Serialize(&serialManager, elem, type);
                        serialManager.writer.Write<uint_s>(0xFFFF);
                    }
                }
            }

            serialManager.writer.SaveToFile(filePath);
            serialManager.Reset();
        }

        void LoadFrom(const char* filePath)
        {
            Destroy();
            *this = {};

            serialManager.SetModeRead();
            serialManager.reader.LoadFromFile(filePath);

            const std::vector<uint32_t> remap = ReadTypeString<NumTypes>(serialManager.reader, ComponentTypeNames);

            const size_t entityCount = serialManager.reader.Read<uint64_t>();
            entityUpdateQueue.resize(entityCount);
            entityLocations.reserve(entityCount);
            for (size_t i = 0; i < entityCount; ++i)
                entityLocations.emplace_back(serialManager.reader.Read<EntityLocation>());

            const size_t archetypeCount = serialManager.reader.Read<uint64_t>();
            archetypeSignatures = ReadBitset<NumTypes>(serialManager.reader, archetypeCount, remap);
            for (uint k = 0; k < archetypeCount; ++k)
            {
                signature archetype = archetypeSignatures[k];
                archetypeSignaturesInv[archetype] = k;

                if (k > 0)
                {
                    std::vector<uint> order;
                    for (size_t type = 0; type < NumTypes; ++type)
                        if (archetype[type])
                            order.push_back(type);
                    archetypeData.emplace_back(order);
                }

                ArchetypeData& data = archetypeData[k];
                const size_t count = serialManager.reader.Read<uint64_t>();
                for (size_t i = 0; i < count; i++)
                    data.IncCount(serialManager.reader.Read<EntityID>());

                // every field not serialized below is default initialized
                data.DefaultAllElems();

                for (size_t j = 0; j < remap.size(); ++j)
                {
                    const size_t type = remap[j];

                    if (!archetype[type])
                        continue;

                    std::byte* byteData = type < NumTypes ? data.data[type].data : nullptr;
                    const size_t stride = type < NumTypes ? data.data[type].stride : 0;
                    for (size_t i = 0; i < count; ++i)
                    {
                        if (byteData)
                            ComponentInfo.Serialize(&serialManager, byteData + i * stride, type);
                        serialManager.SkipToTag(0xFFFF);
                    }
                }
            }

            serialManager.Reset();
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
                archetypeSignaturesInv[archetype] = static_cast<uint>(archetypeData.size());
                archetypeData.emplace_back(updateNewOrder);
                archetypeSignatures.push_back(archetype);
            }
            updateNewSignatures.clear();
            updateNewOrder.clear();

            // move entity to correct container and initialise data
            for (auto [entity, archetype] : updateEntityTypes)
            {
                const uint archetype_index = archetypeSignaturesInv[archetype];
                ArchetypeData& data = archetypeData[archetype_index];

                const EntityLocation old_location = entityLocations[entity];

                ArchetypeData& old_data = archetypeData[old_location.archetype];
                const signature& old_archetype = archetypeSignatures[old_location.archetype];

                std::vector<UpdateInstr>& updateQueue = entityUpdateQueue[entity];

                entityLocations[entity] = { archetype_index, static_cast<uint>(data.GetCount()) };

                data.IncCount(entity);

                signature updating {};

                // init all added components, destroy all removed components
                while (!updateQueue.empty())
                {
                    const auto& [i, init_bytes] = updateQueue.back();
                    updating[i] = true;

                    if (!archetype[i])
                    {
                        // destroy unused added components
                        if (init_bytes)
                            ComponentInfo.Destroy(init_bytes.get(), i);

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
                            ComponentInfo.Destroy(elem, i);
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

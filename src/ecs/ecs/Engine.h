#pragma once
#include <unordered_map>
#include <vector>

#include "Archetype.h"
#include "UpdateQueue.h"

namespace ecs
{
    class Engine;

    template <ComponentType...>
    class EngineView;
    template <ComponentType...>
    class ArchetypeIterator;
    class EngineWidget;

    struct EntityLocation
    {
        uint archetype;
        uint index;
    };

    class Engine
    {
        template <ComponentType...>
        friend class EngineView;
        template <ComponentType...>
        friend class ArchetypeIterator;
        friend class EngineWidget;

        // friendly extension methods
        template <typename... Ts>
        friend void SerializeOnly(Engine& engine, serial::Stream& m);

        friend void Widget(Engine& engine, Entity focus);

        std::vector<Archetype> archetypes = std::vector<Archetype>(1);
        std::vector<EntityLocation> entities;

        std::vector<EntityID> deleted;
        bool anyUpdates = false;
        std::vector<std::vector<UpdateInstr>> entityUpdateQueue {};
        std::unordered_map<size_t, std::vector<uint>> archetypeMap { { std::_Hash_impl::hash(nullptr, 0), { 0 } } };

        uint FindArchetype(const std::vector<TypeID>& types);

        void RawAdd(Entity e, const std::byte* data, const TypeInfo& typeInfo);

        void RawRemove(Entity e, TypeID type);

        void ReadEngineTypes(const char* serialTypes, const std::vector<TypeID>& types, serial::Stream& m);

        void WriteEngineTypes(const char* serialTypes, const std::vector<TypeID>& types, serial::Stream& m) const;

    public:
        Entity New(bool canUseDeleted = true);

        bool IsValid(Entity e) const
        {
            return e < entities.size() && entities[e].index < BadMaxEntity;
        }

        template <ComponentType T>
        void Add(Entity e, const T& data)
        {
            assert(IsValid(e));
            entityUpdateQueue[e].emplace_back(UpdateInstr::Add(data, TypeRegistry::GetID<T>()));
            anyUpdates = true;
        }

        template <ComponentType T>
        void Remove(Entity e)
        {
            assert(IsValid(e));
            entityUpdateQueue[e].emplace_back(UpdateInstr::Remove(TypeRegistry::GetID<T>()));
            anyUpdates = true;
        }

        void RemoveAll(Entity e);

        void Delete(Entity e);

        template <ComponentType T>
        T& Get(Entity e) const
        {
            assert(IsValid(e));
            auto [archetype, index] = entities[e];
            return *reinterpret_cast<T*>(archetypes[archetype].GetElem(index, TypeRegistry::GetID<T>()));
        }

        template <ComponentType... Ts>
        std::tuple<Ts*...> GetMany(Entity e) const
        {
            assert(IsValid(e));
            auto [archetype, index] = entities[e];
            const auto& elems = archetypes[archetype];
            return std::tuple<Ts*...>(
                reinterpret_cast<Ts*>(elems.GetElem(index, TypeRegistry::GetID<Ts>()))...
            );
        }

        template <ComponentType T>
        T* TryGet(Entity e) const
        {
            if (!IsValid(e))
                return nullptr;
            auto [archetype, index] = entities[e];
            return reinterpret_cast<T*>(archetypes[archetype].TryGetElem(index, TypeRegistry::GetID<T>()));
        }

        template <ComponentType... Ts>
        std::tuple<Ts*...> TryGetMany(Entity e) const
        {
            if (!IsValid(e))
                return std::tuple<Ts*...>();
            auto [archetype, index] = entities[e];
            const auto& elems = archetypes[archetype];
            return std::tuple<Ts*...>(
                reinterpret_cast<Ts*>(elems.TryGetElem(index, TypeRegistry::GetID<Ts>()))...
            );
        }

        template <ComponentType T>
        bool Has(Entity e) const
        {
            return IsValid(e) && archetypes[entities[e].archetype].StoresType(TypeRegistry::GetID<T>());
        }

        void Refresh();

        void Destroy();

        void Serialize(serial::Stream& m);

        void Insert(const Engine& other);
    };
}

#pragma once
#include <unordered_set>

#include "ArchetypeData.h"
#include "ComponentTypeInfo.h"
#include "UpdateQueue.h"

// for neatness, it gets a bit messy when they're all the same

#define ECS_TEMPLATE(T) template <ComponentType T> requires one_of_v<T, Components...>
#define ECS_VA_TEMPLATE(Ts) template <ComponentType... Ts> requires (one_of_v<Ts, Components...> && ...)

namespace ecs
{
    struct EntityLocation
    {
        uint archetype;
        uint index;
    };

    template <typename... Components>
    class EntityManager
    {
        static constexpr size_t NumTypes = sizeof...(Components);

        ECS_TEMPLATE(T)
        static constexpr TypeID typeID = index_of_type_v<T, Components...>;

        using signature = std::bitset<NumTypes>;
        using ThisArchetypeData = ArchetypeData<NumTypes>;

        using ThisComponentTypeInfo = ComponentTypeInfo<Components...>;

        inline static ThisComponentTypeInfo componentInfo = ThisComponentTypeInfo::Construct();

        std::vector<ThisArchetypeData> archetypeData = std::vector<ThisArchetypeData>(1);
        std::vector<signature> archetypeSignatures = std::vector<signature>(1);
        std::unordered_map<signature, uint> archetypeSignaturesInv {};

        std::vector<EntityLocation> entityLocations {};

        std::vector<std::vector<UpdateInstr>> entityUpdateQueue {};
        std::unordered_map<EntityID, signature> updateEntityTypes {};
        std::unordered_set<signature> updateNewSignatures {};
        std::vector<TypeInfo> updateNewTypeInfo {};
        EntityID freshCount = 0;


        Entity NewEntity();

        void AddComponent(Entity e, const std::byte* data, TypeID type);

        void Read(serial::Stream& m);
        void Write(serial::Stream& m);

    public:

        EntityManager() = default;
        ~EntityManager() = default;
        EntityManager(const EntityManager& other) = delete;
        EntityManager(EntityManager&& other) noexcept = default;
        EntityManager& operator=(const EntityManager& other) = delete;
        EntityManager& operator=(EntityManager&& other) noexcept = default;

        template <typename T, typename... Ts> requires one_of_v<T, Components...> && (one_of_v<Ts, Components...> && ...)
        Entity NewEntity(const T& data, const Ts&... datas)
        {
            Entity e = NewEntity();
            AddComponent<T>(e, data);
            (AddComponents<Ts>(e, datas), ...);
            return e;
        }

        ECS_TEMPLATE(T)
        T& GetComponent(Entity e)
        {
            assert(HasComponent<T>(e));
            auto [archetype, index] = entityLocations[e];
            return *reinterpret_cast<T*>(archetypeData[archetype].GetElem(index, typeID<T>));
        }

        ECS_VA_TEMPLATE(Ts)
        std::tuple<Ts&...> GetComponents(Entity e)
        {
            assert(HasComponents<Ts...>(e));
            auto [archetype, index] = entityLocations[e];
            const ThisArchetypeData& data = archetypeData[archetype];
            return std::tuple<Ts&...>(*reinterpret_cast<Ts*>(data.GetElem(index, typeID<Ts>))...);
        }

        ECS_TEMPLATE(T)
        void AddComponent(Entity e, const T& data)
        {
            assert(e < entityLocations.size());
            entityUpdateQueue[e].push_back(UpdateInstr::NewAdd(data, typeID<T>));
        }

        ECS_TEMPLATE(T)
        void RemoveComponent(Entity e)
        {
            assert(e < entityLocations.size());
            entityUpdateQueue[e].push_back(UpdateInstr::NewRemove(typeID<T>()));
        }

        ECS_TEMPLATE(T)
        bool HasComponent(Entity e) const
        {
            if (e >= entityLocations.size())
                return false;

            const signature& archetype = archetypeSignatures[entityLocations[e].archetype];
            return archetype.test(typeID<T>);
        }

        ECS_VA_TEMPLATE(Ts)
        void AddComponents(Entity e, const Ts&... data) { (AddComponent<Ts>(e, data), ...); }

        ECS_VA_TEMPLATE(Ts)
        void RemoveComponents(Entity e) { (RemoveComponent<Ts>(e), ...); }

        ECS_VA_TEMPLATE(Ts)
        bool HasComponents(Entity e) const
        {
            if (e >= entityLocations.size())
                return false;

            const signature& archetype = archetypeSignatures[entityLocations[e].archetype];

            return (archetype[typeID<Ts>] && ...);
        }

        bool HasAnyComponent(Entity e) const;

        void RemoveAllComponents(Entity e);

        void Serialize(serial::Stream& m);

        void Destroy();

        void RefreshComponents();

        ECS_VA_TEMPLATE(Ts)
        class EntityView
        {
            EntityManager* manager;

        public:
            explicit EntityView(EntityManager* manager) : manager(manager) {}

            class iterator
            {
                uint entityIndex = 0;
                uint entityCount = 0;
                uint archIndex = 1;
                ThisArchetypeData* nextData = nullptr;
                EntityManager& manager;

                static signature viewSignature;

                void FindNextValidArchetype()
                {
                    std::vector<ThisArchetypeData>& data = manager.archetypeData;
                    const std::vector<signature>& signatures = manager.archetypeSignatures;

                    while (archIndex < data.size())
                    {
                        entityCount = data[archIndex].GetCount();
                        if (entityCount > 0 && (signatures[archIndex] & viewSignature) == viewSignature)
                        {
                            nextData = &data[archIndex];
                            return;
                        }
                        ++archIndex;
                    }
                    archIndex = data.size();
                }

            public:
                iterator(EntityManager& manager, const bool isBegin) : manager(manager)
                {
                    if (isBegin)
                        FindNextValidArchetype();
                    else
                        archIndex = static_cast<uint>(manager.archetypeData.size());
                }

                std::tuple<Entity, Ts&...> operator*() const
                {
                    ThisArchetypeData& data = *nextData;
                    return std::tuple<Entity, Ts&...>(data.entities[entityIndex], *reinterpret_cast<Ts*>(data.GetElem(entityIndex, typeID<Ts>))...);
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

            iterator begin() const { return iterator(*manager, true); }
            iterator end() const { return iterator(*manager, false); }

            std::tuple<Entity, Ts&...> First() const { return *begin(); }
            Entity FirstEntity() const { return std::get<0>(*begin()); }
            std::tuple_element<0, std::tuple<Ts&...>> FirstComponent() const { return std::get<1>(*begin()); }
        };

        ECS_VA_TEMPLATE(Ts)
        EntityView<Ts...> View() { return EntityView<Ts...>(this); }
    };

    template <typename... Components>
    ECS_VA_TEMPLATE(Ts)
    typename EntityManager<Components...>::signature EntityManager<Components...>::EntityView<Ts...>::iterator::viewSignature = []
    {
        signature viewtype;
        (viewtype.set(typeID<Ts>), ...);
        return viewtype;
    }();
}

#undef ECS_TEMPLATE
#undef ECS_VA_TEMPLATE

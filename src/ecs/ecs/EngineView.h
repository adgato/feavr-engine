#pragma once
#include "Engine.h"
#include <algorithm>

namespace ecs
{
    template <ComponentType...>
    struct ViewWith
    {
        ~ViewWith() = delete;
    };

    template <ComponentType...>
    struct ViewWithout
    {
        ~ViewWithout() = delete;
    };

    template <ComponentType... Included>
    class ArchetypeIterator
    {
        const Engine& engine;
        const std::vector<uint>& relevantArchetypes;
        uint relevantIndex;
        uint archIndex = 0;
        EntityID entityIndex = 0;
        EntityID entityCount = 0;

    public:
        explicit ArchetypeIterator(const Engine& engine, const std::vector<uint>& relevantArchetypes, const bool end) : engine(engine), relevantArchetypes(relevantArchetypes)
        {
            for (relevantIndex = end ? relevantArchetypes.size() : 0; relevantIndex < relevantArchetypes.size(); ++relevantIndex)
            {
                archIndex = relevantArchetypes[relevantIndex];
                entityCount = engine.archetypes[archIndex].entities.size();
                if (entityCount > 0)
                    break;
            }
        }

        std::tuple<Entity, Included&...> operator*() const
        {
            const Archetype& data = engine.archetypes[archIndex];
            return std::tuple<Entity, Included&...>(
                data.entities[entityIndex],
                *reinterpret_cast<Included*>(data.GetElem(entityIndex, TypeRegistry::GetID<Included>()))...
            );
        }

        ArchetypeIterator& operator++()
        {
            const uint end = relevantArchetypes.size();
            if (relevantIndex >= end || ++entityIndex < entityCount)
                return *this;
            entityIndex = 0;
            do
            {
                if (++relevantIndex == end)
                    return *this;

                archIndex = relevantArchetypes[relevantIndex];
                entityCount = engine.archetypes[archIndex].entities.size();
            } while (entityCount == 0);
            return *this;
        }

        bool operator==(const ArchetypeIterator& other) const
        {
            return relevantIndex == other.relevantIndex && entityIndex == other.entityIndex;
        }

        bool operator!=(const ArchetypeIterator& other) const
        {
            return relevantIndex != other.relevantIndex || entityIndex != other.entityIndex;
        }
    };

    template <ComponentType... Ts>
    class SortedTypes
    {
        inline static std::array<TypeID, sizeof...(Ts)> types = []
        {
            std::array<TypeID, sizeof...(Ts)> array = { TypeRegistry::GetID<Ts>()... };
            std::sort(array.begin(), array.end());
            return array;
        }();

    public:
        static TypeID Get(size_t index)
        {
            return types[index];
        }
    };

    template <ComponentType... Included>
    class EngineView
    {
    public:
        template <ComponentType... Excluded>
        using Without = EngineView<ViewWith<Included...>, ViewWithout<Excluded...>>;

    private:
        const Engine& engine;
        std::vector<uint> relevantArchetypes {};
        size_t search = 0;

        static bool IsRelevant(const std::vector<TypeID>& superTypes)
        {
            if (superTypes.size() < sizeof...(Included))
                return false;
            size_t j = 0;
            for (size_t i = 0; i < superTypes.size(); ++i)
            {
                if (SortedTypes<Included...>::Get(j) == superTypes[i] && ++j == sizeof...(Included))
                    return true;
            }
            return false;
        }

        void SearchNewArchetypes()
        {
            for (; search < engine.archetypes.size(); ++search)
            {
                if (IsRelevant(engine.archetypes[search].types))
                    relevantArchetypes.emplace_back(search);
            }
        }

    public:
        EngineView() = default;

        explicit EngineView(const Engine& engine) : engine(engine) {}

        ArchetypeIterator<Included...> begin()
        {
            SearchNewArchetypes();
            return ArchetypeIterator<Included...>(engine, relevantArchetypes, false);
        }

        ArchetypeIterator<Included...> end()
        {
            SearchNewArchetypes();
            return ArchetypeIterator<Included...>(engine, relevantArchetypes, true);
        }
    };

    template <ComponentType... Included, ComponentType... Excluded>
    class EngineView<ViewWith<Included...>, ViewWithout<Excluded...>>
    {
        const Engine& engine;
        std::vector<uint> relevantArchetypes {};
        size_t search = 0;

        static bool IsRelevant(const std::vector<TypeID>& superTypes)
        {
            if (superTypes.size() < sizeof...(Included))
                return false;
            size_t j = 0;
            for (size_t i = 0; i < superTypes.size(); ++i)
            {
                if (SortedTypes<Included...>::Get(j) == superTypes[i] && ++j == sizeof...(Included))
                    goto checkexclude;
            }
            return false;
        checkexclude:
            if constexpr (sizeof...(Excluded) > 0)
            {
                size_t k = 0;
                for (size_t i = 0; i < superTypes.size(); ++i)
                {
                    TypeID type = superTypes[i];
                    while (SortedTypes<Excluded...>::Get(k) < type)
                        if (++k >= sizeof...(Excluded))
                            return true;
                    if (SortedTypes<Excluded...>::Get(k) == type)
                        return false;
                }
            }
            return true;
        }

        void SearchNewArchetypes()
        {
            for (; search < engine.archetypes.size(); ++search)
            {
                if (IsRelevant(engine.archetypes[search].types))
                    relevantArchetypes.emplace_back(search);
            }
        }

    public:
        explicit EngineView(const Engine& engine) : engine(engine) {}

        ArchetypeIterator<Included...> begin()
        {
            SearchNewArchetypes();
            return ArchetypeIterator<Included...>(engine, relevantArchetypes, false);
        }

        ArchetypeIterator<Included...> end()
        {
            SearchNewArchetypes();
            return ArchetypeIterator<Included...>(engine, relevantArchetypes, true);
        }
    };
}

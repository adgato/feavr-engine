#pragma once
#include <array>
#include <cassert>

#include "Common.h"
#include "RawArray.h"

namespace ecs
{
    class ArchetypeData
    {
        size_t count = 0;
        size_t capacity = 32;

    public:
        std::array<RawArray, NumTypes> data {};
        std::vector<EntityID> entities {};
        std::vector<uint> types {};

        ArchetypeData() = default;

        ArchetypeData(const std::vector<uint>& order, const std::array<TypeInfo, NumTypes>& typeInfo)
        {
            types = order;
            for (const size_t i : types)
            {
                data[i] = RawArray(typeInfo[i]);
                data[i].Realloc(capacity);
            }
        }

        std::byte* GetElem(const uint index, const uint order) const
        {
            assert(index < count);
            assert(data[order].data);
            return data[order].GetElem(index);
        }

        void SetElem(const uint order, const std::byte* src)
        {
            assert(data[order].data);
            data[order].SetElem(count - 1, src);
        }

        size_t GetCount() const
        {
            return count;
        }

        void IncCount(Entity entity)
        {
            entities.push_back(entity);

            if (++count < capacity)
                return;

            size_t new_capacity = 4 * capacity;

            bool badalloc = false;
            for (const uint i : types)
            {
                if (data[i].Realloc(new_capacity))
                    continue;
                badalloc = true;
                break;
            }

            if (badalloc)
            {
                new_capacity = 2 * capacity;
                for (const uint i : types)
                {
                    if (!data[i].Realloc(new_capacity))
                        throw std::bad_alloc();
                }
            }

            capacity = new_capacity;
        }

        Entity RemoveElem(const uint index)
        {
            assert(index < count);

            count--;
            if (index < count)
            {
                for (const uint i : types)
                    data[i].ReplaceElem(index, count);
            }

            Entity last = entities.back();
            entities[index] = last;
            entities.pop_back();
            return last;
        }
    };
}
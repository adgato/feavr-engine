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
        std::vector<TypeID> types {};

        ArchetypeData() = default;

        explicit ArchetypeData(const std::vector<TypeID>& type)
        {
            types = type;
            for (const size_t i : types)
            {
                data[i] = RawArray(ComponentInfo.GetTypeInfo(i));
                data[i].Realloc(capacity, 0);
            }
        }

        std::byte* GetElem(const uint index, const TypeID type) const
        {
            assert(index < count && data[type].data);
            return data[type].GetElem(index);
        }

        void SetElem(const TypeID type, const std::byte* src) const
        {
            assert(data[type].data);
            data[type].SetElem(count - 1, src);
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
                if (data[i].Realloc(new_capacity, capacity))
                    continue;
                badalloc = true;
                break;
            }

            if (badalloc)
            {
                new_capacity = 2 * capacity;
                for (const uint i : types)
                {
                    if (!data[i].Realloc(new_capacity, capacity))
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
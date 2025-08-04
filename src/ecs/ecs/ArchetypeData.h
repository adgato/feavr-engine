#pragma once
#include <array>
#include <vector>
#include <cassert>

#include "RawArray.h"

namespace ecs
{
    template <size_t NumTypes>
    class ArchetypeData
    {
        size_t count = 0;
        size_t capacity = 32;

    public:
        std::array<RawArray, NumTypes> data {};
        std::vector<EntityID> entities {};
        std::vector<TypeID> types {};

        ArchetypeData() = default;

        explicit ArchetypeData(const std::vector<TypeInfo>& typesInfo)
        {
            types.reserve(typesInfo.size());
            for (size_t i = 0; i < typesInfo.size(); ++i)
            {
                const TypeInfo& info = typesInfo[i];
                TypeID type = info.type;

                types.push_back(type);
                data[type] = RawArray(info);
                data[type].Realloc(capacity, 0);
            }
        }

        std::byte* GetElem(const uint index, const TypeID type) const
        {
            assert(index < count);
            return data[type].GetElem(index);
        }

        void SetElem(const TypeID type, const std::byte* src) const
        {
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

            const size_t new_capacity = capacity + (capacity >> 1);

            for (const uint i : types)
                if (!data[i].Realloc(new_capacity, capacity))
                    throw std::bad_alloc();

            capacity = new_capacity;
            assert(count <= capacity);
        }

        Entity RemoveElem(const uint index)
        {
            assert(index < count);
            assert(count > 0);

            if (index < --count)
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
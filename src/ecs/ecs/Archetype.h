#pragma once
#include <vector>

#include "RawArray.h"
#include "TypeRegistry.h"

namespace ecs
{
    class Archetype
    {
        size_t count = 0;
        size_t capacity = 32;

        TypeID minType = BadMaxType;
        TypeID maxType = 0;

    public:
        std::vector<RawArray> data {};
        std::vector<EntityID> entities {};
        std::vector<TypeID> globalTypes {};
        // ordered vector
        std::vector<TypeID> types {};

        Archetype() = default;
        Archetype(const Archetype& other) = delete;
        Archetype(Archetype&& other) noexcept = default;
        Archetype& operator=(const Archetype& other) = delete;
        Archetype& operator=(Archetype&& other) noexcept = default;

        explicit Archetype(const std::vector<TypeID>& types);

        bool StoresType(const TypeID type) const
        {
            return type >= minType && type <= maxType && globalTypes[type - minType] < BadMaxType;
        }

        std::byte* GetElem(const uint index, const TypeID type) const
        {
            assert(index < count && StoresType(type));
            return data[globalTypes[type - minType]].GetElem(index);
        }

        void SetElem(const TypeID type, const std::byte* src) const
        {
            assert(StoresType(type));
            data[globalTypes[type - minType]].SetElem(count - 1, src);
        }

        size_t GetCount() const
        {
            return count;
        }

        void IncCount(Entity entity);

        Entity RemoveElem(uint index);
    };
}

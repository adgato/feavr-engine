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

        bool StoresType(TypeID type) const;

        std::byte* GetElem(uint index, TypeID type) const;

        void SetElem(TypeID type, const std::byte* src) const;

        size_t GetCount() const;

        void IncCount(Entity entity);

        Entity RemoveElem(uint index);
    };
}

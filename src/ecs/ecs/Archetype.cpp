#include "Archetype.h"

namespace ecs
{
    Archetype::Archetype(const std::vector<TypeID>& types)
    {
        this->types = types;

        if (types.size() == 0)
            return;

        data.reserve(types.size());
        for (size_t i = 0; i < types.size(); ++i)
        {
            const TypeInfo& info = TypeRegistry::GetInfo(types[i]);
            const TypeID type = info.type;

            if (type < minType)
                minType = type;
            if (type > maxType)
                maxType = type;

            data.emplace_back(info).Realloc(capacity);
        }

        globalTypes.resize(maxType - minType + 1, BadMaxType);

        for (size_t i = 0; i < types.size(); ++i)
            globalTypes[types[i] - minType] = i;
    }

    bool Archetype::StoresType(const TypeID type) const
    {
        return type >= minType && type <= maxType && globalTypes[type - minType] < BadMaxType;
    }

    std::byte* Archetype::GetElem(const uint index, const TypeID type) const
    {
        assert(index < count && StoresType(type));
        return data[globalTypes[type - minType]].GetElem(index);
    }

    void Archetype::SetElem(const TypeID type, const std::byte* src) const
    {
        assert(StoresType(type));
        data[globalTypes[type - minType]].SetElem(count - 1, src);
    }

    size_t Archetype::GetCount() const
    {
        return count;
    }

    void Archetype::IncCount(Entity entity)
    {
        entities.push_back(entity);

        if (++count < capacity)
            return;

        capacity += capacity >> 1;
        assert(count <= capacity);

        for (RawArray& array : data)
            if (!array.Realloc(capacity))
                throw std::bad_alloc();
    }

    Entity Archetype::RemoveElem(const uint index)
    {
        assert(index < count);
        assert(count > 0);

        if (index < --count)
        {
            for (RawArray& array : data)
                array.ReplaceElem(index, count);
        }

        Entity last = entities.back();
        entities[index] = last;
        entities.pop_back();
        return last;
    }
}

#include "Engine.h"

#include <algorithm>

namespace ecs
{
    static TypeID NextType(const std::vector<TypeID>& types, ssize_t& i)
    {
        return ++i < types.size() ? types[i] : BadMaxType;
    }

    static TypeID NextType(const std::vector<UpdateInstr>& types, ssize_t& i)
    {
        return ++i < types.size() ? types[i].type : BadMaxType;
    }

    static bool TypesEqual(const std::vector<TypeID>& a, const std::vector<TypeID>& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i])
                return false;
        return true;
    }

    static void PushUpdate(std::vector<UpdateInstr>& updateTypes, UpdateInstr&& instr)
    {
        for (size_t i = 0; i < updateTypes.size(); ++i)
        {
            UpdateInstr& replace = updateTypes[i];
            if (instr.type == replace.type)
            {
                if (replace.data)
                    TypeRegistry::Destroy(replace.type, replace.data.get());
                replace = std::move(instr);
                return;
            }
            if (instr.type < replace.type)
            {
                updateTypes.insert(updateTypes.begin() + i, std::move(instr));
                return;
            }
        }
        updateTypes.emplace_back(std::move(instr));
    }

    uint Engine::FindArchetype(const std::vector<TypeID>& types)
    {
        const size_t hash = std::_Hash_impl::hash(types.data(), types.size() * sizeof(TypeID));

        uint newArchetypeIdx = BadMaxIndex;
        if (archetypeMap.contains(hash))
        {
            for (const uint archetype : archetypeMap[hash])
                if (TypesEqual(types, archetypes[archetype].types))
                {
                    newArchetypeIdx = archetype;
                    break;
                }
        }

        if (newArchetypeIdx == BadMaxIndex)
        {
            newArchetypeIdx = archetypes.size();
            archetypeMap[hash].emplace_back(newArchetypeIdx);
            archetypes.emplace_back(Archetype(types));
        }

        return newArchetypeIdx;
    }

    void Engine::RawAdd(Entity e, const std::byte* data, const TypeInfo& typeInfo)
    {
        assert(IsValid(e));
        entityUpdateQueue[e].emplace_back(UpdateInstr::RawAdd(data, typeInfo.type, typeInfo.size));
    }

    Entity Engine::New(const bool canUseDeleted /* = true*/)
    {
        EntityID e;
        if (canUseDeleted && deleted.size() > 0)
        {
            e = deleted.back();
            entities[e].index = archetypes[0].GetCount();
            archetypes[0].IncCount(e);
            deleted.pop_back();
        } else
        {
            e = static_cast<EntityID>(entities.size());
            entities.emplace_back(0, archetypes[0].GetCount());
            archetypes[0].IncCount(e);
            entityUpdateQueue.emplace_back();
        }
        return e;
    }

    bool Engine::IsValid(Entity e) const
    {
        return e < entities.size() && entities[e].index < BadMaxIndex;
    }

    void Engine::RemoveAll(Entity e)
    {
        assert(IsValid(e));
        auto [archetype, index] = entities[e];
        auto& updateQueue = entityUpdateQueue[e];
        for (const TypeID type : archetypes[archetype].types)
            updateQueue.emplace_back(UpdateInstr::Remove(type));
    }

    void Engine::Delete(Entity e)
    {
        assert(IsValid(e));
        entityUpdateQueue[e].emplace_back(UpdateInstr::Remove(BadMaxType));
    }

    void Engine::Destroy()
    {
        for (EntityID e = 0; e < entities.size(); ++e)
            if (IsValid(e))
                Delete(e);
        Refresh();
        entities.clear();
        deleted.clear();
        entityUpdateQueue.clear();
    }

    void Engine::Refresh()
    {
        std::vector<TypeID> newTypes {};
        std::vector<std::byte*> newData {};
        std::vector<UpdateInstr> updateTypes {};

        for (EntityID e = 0; e < entityUpdateQueue.size(); ++e)
        {
            auto& updateQueue = entityUpdateQueue[e];
            if (updateQueue.size() == 0)
                continue;

            newTypes.clear();
            newData.clear();
            updateTypes.clear();

            bool deleteEntity = false;
            for (size_t i = 0; i < updateQueue.size(); ++i)
            {
                if (updateQueue[i].type == BadMaxType)
                    deleteEntity = true;
                else
                    PushUpdate(updateTypes, std::move(updateQueue[i]));
            }
            updateQueue.clear();

            const auto [currentArchetypeIdx, currentIndex] = entities[e];
            const auto& currentArchetype = archetypes[currentArchetypeIdx];

            ssize_t i = -1, j = -1;
            TypeID ti = NextType(currentArchetype.types, i);
            TypeID tj = NextType(updateTypes, j);
            while (ti < BadMaxType || tj < BadMaxType)
            {
                if (ti < tj)
                {
                    newTypes.emplace_back(ti);
                    newData.emplace_back(currentArchetype.GetElem(currentIndex, ti));
                    ti = NextType(currentArchetype.types, i);
                    continue;
                }

                if (ti == tj)
                {
                    TypeRegistry::Destroy(ti, currentArchetype.GetElem(currentIndex, ti));
                    ti = NextType(currentArchetype.types, i);
                }

                if (updateTypes[j].data)
                {
                    newTypes.emplace_back(tj);
                    newData.emplace_back(updateTypes[j].data.get());
                }

                tj = NextType(updateTypes, j);
            }

            const uint newArchetypeIdx = deleteEntity ? 0 : FindArchetype(newTypes);

            if (deleteEntity)
            {
                entities[e] = { 0, BadMaxIndex };
                deleted.push_back(e);
                for (size_t k = 0; k < newTypes.size(); ++k)
                    TypeRegistry::Destroy(newTypes[k], newData[k]);
            } else
            {
                Archetype& newArchetype = archetypes[newArchetypeIdx];
                entities[e] = { newArchetypeIdx, static_cast<uint>(newArchetype.GetCount()) };
                newArchetype.IncCount(e);
                for (size_t k = 0; k < newTypes.size(); ++k)
                    newArchetype.SetElem(newTypes[k], newData[k]);
            }

            Entity moved = archetypes[currentArchetypeIdx].RemoveElem(currentIndex);
            if (e != moved || !deleteEntity && currentArchetypeIdx == newArchetypeIdx)
                entities[moved].index = currentIndex;
        }
    }

    static std::vector<size_t> split(std::string&& s, const std::string& delimiter)
    {
        size_t pos_start = 0;
        size_t pos_end;
        const size_t delim_len = delimiter.length();
        std::vector<size_t> res {};

        if (s.ends_with('\0'))
            s.erase(s.find_first_of('\0'));

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            std::string token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(std::hash<std::string> {}(token));
        }

        res.push_back(std::hash<std::string> {}(s.substr(pos_start)));
        return res;
    }

    void Engine::ReadEngineTypes(const char* serialTypes, const std::vector<TypeID>& types, serial::Stream& m)
    {
        assert(m.reading);
        const size_t numTypes = types.size();
        const auto targetSplit = split(m.reader.ReadString(), ", ");
        const auto sourceSplit = split(serialTypes, ", ");

        const uint targetNumTypes = targetSplit.size();
        assert(sourceSplit.size() == numTypes);

        std::vector remap(targetNumTypes, BadMaxType);
        for (size_t i = 0; i < targetSplit.size(); ++i)
            for (size_t j = 0; j < numTypes; ++j)
                if (targetSplit[i] == sourceSplit[j])
                {
                    remap[i] = j;
                    break;
                }

        std::vector<std::unique_ptr<std::byte[]>> defaultTypeData;
        defaultTypeData.reserve(numTypes);
        for (const TypeID globalType : types)
            defaultTypeData.emplace_back(std::make_unique<std::byte[]>(TypeRegistry::GetInfo(globalType).size));

        EntityID unionOffset = entities.size();
        std::any outerData = unionOffset;
        m.userData.swap(outerData);

        const EntityID entityCount = m.reader.Read<EntityID>();
        for (EntityID i = 0; i < entityCount; ++i)
        {
            Entity e = New(false);
            assert(e == i + unionOffset);

            for (uint numOwn = m.reader.Read<uint>(); numOwn > 0; --numOwn)
            {
                const TypeID retype = m.reader.Read<TypeID>();
                const size_t size = m.reader.Read<serial::fsize>();
                const TypeID type = retype >= targetNumTypes ? BadMaxType : remap[retype];

                if (type == BadMaxType)
                    Delete(e);

                if (type >= numTypes)
                {
                    m.reader.Jump(size);
                    continue;
                }
                const TypeID globalType = types[type];

                std::byte* data = defaultTypeData[type].get();
                TypeRegistry::CopyDefault(globalType, data);
                TypeRegistry::Serialize(globalType, data, m);
                RawAdd(e, data, TypeRegistry::GetInfo(globalType));
            }
        }
        m.userData.swap(outerData);
    }

    void Engine::WriteEngineTypes(const char* serialTypes, const std::vector<TypeID>& types, serial::Stream& m) const
    {
        assert(!m.reading);
        m.writer.WriteString(serialTypes);

        std::vector<TypeID> sortedTypes = types;
        std::sort(sortedTypes.begin(), sortedTypes.end());

        std::vector<TypeID> intersect {};

        const EntityID entityCount = entities.size();
        m.writer.Write<EntityID>(entityCount);

        for (EntityID entity = 0; entity < entityCount; ++entity)
        {
            if (!IsValid(entity))
            {
                m.writer.Write<uint>(1);
                m.writer.Write<TypeID>(BadMaxType);
                m.writer.Write<serial::fsize>(0);
                continue;
            }

            const auto& [archetype, index] = entities[entity];
            const Archetype& data = archetypes[archetype];

            intersect.clear();
            size_t i = 0, j = 0;
            while (i < sortedTypes.size() && j < data.types.size())
            {
                const TypeID ti = sortedTypes[i];
                const TypeID tj = data.types[j];
                if (ti == tj)
                {
                    intersect.emplace_back(ti);
                    ++i;
                    ++j;
                } else if (ti < tj) ++i;
                else ++j;
            }

            m.writer.Write<uint>(intersect.size());
            for (TypeID type : intersect)
            {
                m.writer.Write<TypeID>(type);
                const size_t offset = m.writer.Reserve<serial::fsize>();

                TypeRegistry::Serialize(type, data.GetElem(index, type), m);

                m.writer.WriteOver<serial::fsize>(m.writer.GetCount() - offset - sizeof(serial::fsize), offset);
            }
        }
    }

    void Engine::Union(const Engine& other)
    {
        const size_t count = TypeRegistry::RegisteredCount();
        std::string serialTypes {};
        std::vector<TypeID> types(count);
        for (TypeID i = 0; i < count; ++i)
        {
            types[i] = TypeRegistry::GetInfo(i).type;
            serialTypes += std::to_string(types[i]);
            if (i < count - 1)
                serialTypes += ", ";
        }

        serial::Stream w;
        w.InitWrite();
        other.WriteEngineTypes(serialTypes.c_str(), types, w);

        serial::Stream r;
        r.InitRead();
        r.reader.ViewFrom(w.writer.AsSpan());
        ReadEngineTypes(serialTypes.c_str(), types, r);
    }
}

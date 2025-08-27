#include "Engine.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>


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

        uint newArchetypeIdx = BadMaxEntity;
        if (archetypeMap.contains(hash))
        {
            for (const uint archetype : archetypeMap[hash])
                if (TypesEqual(types, archetypes[archetype].types))
                {
                    newArchetypeIdx = archetype;
                    break;
                }
        }

        if (newArchetypeIdx == BadMaxEntity)
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
        anyUpdates = true;
    }

    void Engine::RawRemove(Entity e, const TypeID type)
    {
        assert(IsValid(e));
        entityUpdateQueue[e].emplace_back(UpdateInstr::Remove(type));
        anyUpdates = true;
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
            entitySources.emplace_back();
        }
        return e;
    }

    void Engine::RemoveAll(Entity e)
    {
        assert(IsValid(e));
        auto [archetype, index] = entities[e];
        auto& updateQueue = entityUpdateQueue[e];
        for (const TypeID type : archetypes[archetype].types)
            updateQueue.emplace_back(UpdateInstr::Remove(type));
        anyUpdates = true;
    }

    void Engine::Delete(Entity e)
    {
        assert(IsValid(e));
        entityUpdateQueue[e].emplace_back(UpdateInstr::Remove(BadMaxType));
        anyUpdates = true;
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
        entitySources.clear();
    }

    void Engine::Refresh()
    {
        if (!anyUpdates)
            return;
        anyUpdates = false;

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
                entities[e] = { 0, BadMaxEntity };
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

    static std::vector<std::string> split(std::string&& s, const std::string& delimiter)
    {
        size_t pos_start = 0;
        size_t pos_end;
        const size_t delim_len = delimiter.length();
        std::vector<std::string> res {};

        if (s.ends_with('\0'))
            s.erase(s.find_first_of('\0'));

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            std::string token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    serial::Stream Engine::AddFrom(const assets_system::AssetID assetID, const bool useAsSource /* = true*/)
    {
        const size_t count = TypeRegistry::RegisteredCount();
        std::string serialTypes {};
        for (TypeID i = 0; i < count; ++i)
        {
            serialTypes += TypeRegistry::GetInfo(i).name;
            if (i < count - 1)
                serialTypes += "; ";
        }

        assets_system::AssetFile assetFile = assets_system::AssetManager::LoadAsset(assetID);
        assert(assetFile.HasFormat("SCNE", 0));

        serial::Stream m = assetFile.ReadFromBlob();

        // std::unordered_map<assets_system::AssetID, Scene> sourceMap {};
        const TypeID numTypes = TypeRegistry::RegisteredCount();

        const auto targetSplit = split(m.reader.ReadString(), "; ");
        const auto sourceSplit = split(serialTypes.c_str(), "; ");

        const uint targetNumTypes = targetSplit.size();
        assert(sourceSplit.size() == numTypes);

        std::vector<size_t> remap(targetNumTypes, BadMaxType);
        std::vector sourceUsed(numTypes, false);

        for (size_t i = 0; i < targetSplit.size(); ++i)
            for (size_t j = 0; j < numTypes; ++j)
                if (targetSplit[i] == sourceSplit[j])
                {
                    remap[i] = j;
                    sourceUsed[j] = true;
                    break;
                }

        for (size_t i = 0; i < remap.size(); ++i)
            if (remap[i] == BadMaxType)
                fmt::println("Note: Unknown type {}", targetSplit[i]);

        for (size_t j = 0; j < sourceUsed.size(); ++j)
            if (!sourceUsed[j])
                fmt::println("Note: Ignoring type {}", sourceSplit[j]);

        std::vector<std::unique_ptr<std::byte[]>> defaultTypeData;
        defaultTypeData.reserve(numTypes);
        for (TypeID globalType = 0; globalType < numTypes; ++globalType)
            defaultTypeData.emplace_back(std::make_unique<std::byte[]>(TypeRegistry::GetInfo(globalType).size));

        EntityID unionOffset = entities.size();
        std::any outerData = unionOffset;
        m.userData.swap(outerData);

        const EntityID entityCount = m.reader.Read<EntityID>();

        std::vector<EntitySource> oldEntitySources;
        oldEntitySources.resize(entityCount);
        for (size_t i = 0; i < oldEntitySources.size(); ++i)
            oldEntitySources[i].Serialize(m);

        for (EntityID i = 0; i < entityCount; ++i)
        {
            Entity e = New(false);
            assert(e == i + unionOffset);

            EntitySource& entitySource = entitySources.back();
            if (useAsSource)
            {
                entitySource.target = i;
                entitySource.asset = assetID;
            } else
            {
                entitySource = oldEntitySources[i];
                for (TypeID& oldType : entitySource.types)
                    oldType = remap[oldType];
            }

            for (TypeID numOwn = m.reader.Read<TypeID>(); numOwn > 0; --numOwn)
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

                std::byte* data = defaultTypeData[type].get();
                TypeRegistry::CopyDefault(type, data);
                TypeRegistry::Serialize(type, data, m);
                RawAdd(e, data, TypeRegistry::GetInfo(type));
            }
        }
        m.userData.swap(outerData);
        return m;
    }

    void Engine::WriteTo(serial::Stream& m)
    {
        const size_t count = TypeRegistry::RegisteredCount();
        std::string serialTypes {};
        for (TypeID i = 0; i < count; ++i)
        {
            serialTypes += TypeRegistry::GetInfo(i).name;
            if (i < count - 1)
                serialTypes += "; ";
        }

        assert(!m.reading);

        m.writer.WriteString(serialTypes);

        const EntityID entityCount = entities.size();
        m.writer.Write<EntityID>(entityCount);

        for (size_t i = 0; i < entityCount; ++i)
            entitySources[i].Serialize(m);

        for (EntityID entity = 0; entity < entityCount; ++entity)
        {
            if (!IsValid(entity))
            {
                m.writer.Write<TypeID>(1);
                m.writer.Write<TypeID>(BadMaxType);
                m.writer.Write<serial::fsize>(0);
                continue;
            }

            const auto& [archetype, index] = entities[entity];
            const Archetype& data = archetypes[archetype];

            m.writer.Write<TypeID>(data.types.size());
            for (const TypeID type : data.types)
            {
                m.writer.Write<TypeID>(type);
                const size_t offset = m.writer.Reserve<serial::fsize>();

                TypeRegistry::Serialize(type, data.GetElem(index, type), m);

                m.writer.WriteOver<serial::fsize>(m.writer.GetCount() - offset - sizeof(serial::fsize), offset);
            }
        }
    }

    void Engine::Lock()
    {
        std::unordered_set<std::byte*> dontDestroy {};
        EntitySource::EngineMap engineMap {};
        for (EntityID e = 0; e < entities.size(); ++e)
        {
            EntitySource& entitySource = entitySources[e];

            if (!IsValid(e))
            {
                entitySource.types.clear();
                continue;
            }
            auto& [archetype, index] = entities[e];
            auto& elem = archetypes[archetype];

            entitySource.types.assign(elem.types.begin(), elem.types.end());

            for (const TypeID type : entitySource.types)
            {
                std::byte* dest = elem.GetElem(index, type);
                if (std::byte* src = entitySource.TryGetFromSource(engineMap, type))
                {
                    TypeRegistry::Destroy(type, dest);
                    std::memcpy(dest, src, TypeRegistry::GetInfo(type).size);
                    dontDestroy.insert(src);
                }
            }
        }

        // manual cleanup (needed as we copied some source components, don't want to destroy these)
        for (auto& pair : engineMap)
        {
            Engine& engine = pair.second;
            for (EntityID e = 0; e < engine.entities.size(); ++e)
            {
                if (!IsValid(e))
                    continue;
                auto& [archetype, index] = engine.entities[e];
                auto& elem = engine.archetypes[archetype];
                for (const TypeID type : elem.types)
                {
                    std::byte* data = elem.GetElem(index, type);
                    if (!dontDestroy.contains(data))
                        TypeRegistry::Destroy(type, data);
                }
            }
        }
    }

    void Engine::Unlock()
    {
        for (EntityID e = 0; e < entities.size(); ++e)
            entitySources[e].types.clear();
    }

    void Engine::Lock(Entity e, const TypeID type)
    {
        assert(IsValid(e));

        EntitySource::EngineMap engineMap {};
        auto& [archetype, index] = entities[e];
        std::byte* dest = archetypes[archetype].GetElem(index, type);
        const std::byte* src = entitySources[e].TryGetFromSource(engineMap, type);
        if (src)
        {
            TypeRegistry::Destroy(type, dest);
            std::memcpy(dest, src, TypeRegistry::GetInfo(type).size);
        }

        // manual cleanup (needed as we copied a source component, don't want to destroy it)
        for (auto& pair : engineMap)
        {
            Engine& engine = pair.second;
            for (EntityID entity = 0; entity < engine.entities.size(); ++entity)
            {
                if (!IsValid(entity))
                    continue;
                auto& [archetype, index] = engine.entities[entity];
                auto& elem = engine.archetypes[archetype];
                for (const TypeID elemType : elem.types)
                {
                    std::byte* data = elem.GetElem(index, elemType);
                    if (src != data)
                        TypeRegistry::Destroy(elemType, data);
                }
            }
        }
    }

    void Engine::Unlock(Entity e, const TypeID type)
    {
        std::erase(entitySources[e].types, type);
    }
}

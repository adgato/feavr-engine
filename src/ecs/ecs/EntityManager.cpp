#include "EntityManager.h"
#include "serialisation/Stream.h"

namespace ecs
{
    std::vector<size_t> split(std::string&& s, const std::string& delimiter)
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

    template <typename... Components>
    Entity EntityManager<Components...>::NewEntity()
    {
        ThisArchetypeData& cleanEntities = archetypeData[0];
        EntityID e;
        if (freshCount == cleanEntities.GetCount())
        {
            e = static_cast<EntityID>(entityLocations.size());
            entityLocations.push_back({ 0, freshCount });
            entityUpdateQueue.emplace_back();
            cleanEntities.IncCount(e);
        } else
            e = cleanEntities.entities[cleanEntities.GetCount() - freshCount - 1];
        freshCount++;

        return e;
    }

    template <typename... Components>
    void EntityManager<Components...>::AddComponent(Entity e, const std::byte* data, const TypeID type)
    {
        assert(e < entityLocations.size());
        entityUpdateQueue[e].push_back(UpdateInstr::NewRawAdd(data, type, componentInfo.GetTypeInfo(type).size));
    }

    template <typename ... Components>
    void EntityManager<Components...>::Read(serial::Stream& m)
    {
        Destroy();
        *this = {};

        const auto targetSplit = split(m.reader.ReadString(), ", ");
        const auto sourceSplit = split(componentInfo.GetTypeNames(), ", ");

        const uint targetNumTypes = targetSplit.size();
        assert(sourceSplit.size() == NumTypes);

        std::vector remap(targetNumTypes, NumTypes);
        for (size_t i = 0; i < targetSplit.size(); ++i)
            for (size_t j = 0; j < NumTypes; ++j)
                if (targetSplit[i] == sourceSplit[j])
                {
                    remap[i] = j;
                    break;
                }

        std::vector<std::unique_ptr<std::byte[]>> components;
        components.reserve(NumTypes);
        for (size_t i = 0; i < NumTypes; ++i)
            components.emplace_back(std::make_unique<std::byte[]>(componentInfo.GetTypeInfo(i).size));

        const EntityID entityCount = m.reader.Read<EntityID>();
        for (EntityID entity = 0; entity < entityCount; ++entity)
        {
            Entity i = NewEntity();
            assert(i == entity);

            for (uint componentCount = m.reader.Read<uint>(); componentCount > 0; --componentCount)
            {
                const TypeID retype = m.reader.Read<TypeID>();
                const size_t size = m.reader.Read<serial::fsize>();
                const TypeID type = remap[retype];
                if (type >= NumTypes)
                {
                    m.reader.Jump(size);
                    continue;
                }

                std::byte* data = components[type].get();
                componentInfo.CopyDefault(data, type);
                componentInfo.Serialize(m, data, type);
                AddComponent(entity, data, type);
            }
        }
        RefreshComponents();
    }

    template <typename ... Components>
    void EntityManager<Components...>::Write(serial::Stream& m)
    {
        RefreshComponents();

        m.writer.WriteString(componentInfo.GetTypeNames());

        const EntityID entityCount = entityLocations.size();
        m.writer.Write<EntityID>(entityCount);

        for (EntityID entity = 0; entity < entityCount; ++entity)
        {
            const auto& [archetype, index] = entityLocations[entity];

            ThisArchetypeData& data = archetypeData[archetype];
            signature entityComponents = archetypeSignatures[archetype];

            uint componentCount = 0;
            for (uint j = 0; j < NumTypes; ++j)
                if (entityComponents[j])
                    componentCount++;

            m.writer.Write<uint>(componentCount);

            for (TypeID type = 0; type < NumTypes; ++type)
            {
                if (!entityComponents[type])
                    continue;

                m.writer.Write<TypeID>(type);
                const size_t offset = m.writer.Reserve<serial::fsize>();

                componentInfo.Serialize(m, data.GetElem(index, type), type);

                m.writer.WriteOver<serial::fsize>(m.writer.GetCount() - offset - sizeof(serial::fsize), offset);
            }
        }
    }

    template <typename... Components>
    bool EntityManager<Components...>::HasAnyComponent(Entity e) const
    {
        return e < entityLocations.size() && archetypeSignatures[entityLocations[e].archetype].any();
    }

    template <typename... Components>
    void EntityManager<Components...>::RemoveAllComponents(Entity e)
    {
        if (e >= entityLocations.size())
            return;

        const signature archetype = archetypeSignatures[entityLocations[e].archetype];
        for (uint type = 0; type < NumTypes; ++type)
        {
            if (archetype.test(type))
                entityUpdateQueue[e].push_back(UpdateInstr::NewRemove(type));
        }
    }

    template <typename ... Components>
    void EntityManager<Components...>::Serialize(serial::Stream& m)
    {
        if (m.loading)
            Read(m);
        else
            Write(m);
    }

    template <typename... Components>
    void EntityManager<Components...>::Destroy()
    {
        for (EntityID e = 0; e < entityLocations.size(); ++e)
            RemoveAllComponents(e);
        RefreshComponents();
    }

    template <typename... Components>
    void EntityManager<Components...>::RefreshComponents()
    {
        freshCount = 0;

        // create map from entities to their new archetypes
        for (EntityID entity = 0; entity < entityUpdateQueue.size(); ++entity)
            for (auto& [type, data] : entityUpdateQueue[entity])
            {
                if (!updateEntityTypes.contains(entity))
                    updateEntityTypes[entity] = archetypeSignatures[entityLocations[entity].archetype];

                // data == null means remove component
                updateEntityTypes[entity].set(type, static_cast<bool>(data));
            }

        // find new archetypes
        for (const auto& updateType : updateEntityTypes)
            updateNewSignatures.insert(updateType.second);
        for (const signature& archetype : archetypeSignatures)
            updateNewSignatures.erase(archetype);

        // create new archetype data holders
        for (const signature& archetype : updateNewSignatures)
        {
            updateNewTypeInfo.clear();
            for (uint type = 0; type < NumTypes; ++type)
            {
                if (archetype.test(type))
                    updateNewTypeInfo.push_back(componentInfo.GetTypeInfo(type));
            }
            archetypeSignaturesInv[archetype] = static_cast<uint>(archetypeData.size());
            archetypeData.emplace_back(updateNewTypeInfo);
            archetypeSignatures.push_back(archetype);
        }
        updateNewSignatures.clear();
        updateNewTypeInfo.clear();

        // move entity to correct container and initialise data
        for (auto [entity, archetype] : updateEntityTypes)
        {
            const uint archetype_index = archetypeSignaturesInv[archetype];
            ThisArchetypeData& data = archetypeData[archetype_index];

            const EntityLocation old_location = entityLocations[entity];

            ThisArchetypeData& old_data = archetypeData[old_location.archetype];
            const signature& old_archetype = archetypeSignatures[old_location.archetype];

            std::vector<UpdateInstr>& updateQueue = entityUpdateQueue[entity];

            entityLocations[entity] = { archetype_index, static_cast<uint>(data.GetCount()) };

            data.IncCount(entity);

            signature updating {};

            // init all added components, destroy all removed components
            while (!updateQueue.empty())
            {
                const auto& [i, init_bytes] = updateQueue.back();
                updating.set(i);

                if (!archetype.test(i))
                {
                    // destroy unused added components
                    if (init_bytes)
                        componentInfo.Destroy(init_bytes.get(), i);

                    updateQueue.pop_back();
                    continue;
                }

                // this type will be added, init with most recent update data
                archetype.set(i, false);
                data.SetElem(i, init_bytes.get());

                updateQueue.pop_back();
            }

            // move or destroy all existing components
            for (uint i = 0; i < NumTypes; ++i)
                if (old_archetype.test(i))
                {
                    std::byte* elem = old_data.GetElem(old_location.index, i);
                    if (updating.test(i))
                        componentInfo.Destroy(elem, i);
                    else
                        data.SetElem(i, elem);
                }

            Entity moved = old_data.RemoveElem(old_location.index);
            if (entity != moved)
                entityLocations[moved].index = old_location.index;
        }
        updateEntityTypes.clear();
    }


}

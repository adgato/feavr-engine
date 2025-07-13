#include "Engine.h"

namespace ecs
{
    Entity EntityManager::NewEntity()
    {
        ArchetypeData& cleanEntities = archetypeData[0];
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

    void EntityManager::Destroy()
    {
        for (EntityID e = 0; e < entityLocations.size(); ++e)
            RemoveAllComponents(e);
        RefreshComponents();
        serialManager.Destroy();
    }

    void EntityManager::RefreshComponents()
    {
        freshCount = 0;

        // create map from entities to their new archetypes
        for (EntityID entity = 0; entity < entityUpdateQueue.size(); ++entity)
            for (auto& [type, data] : entityUpdateQueue[entity])
            {
                if (!updateEntityTypes.contains(entity))
                    updateEntityTypes[entity] = archetypeSignatures[entityLocations[entity].archetype];

                // data == null means remove component
                updateEntityTypes[entity][type] = static_cast<bool>(data);
            }

        // find new archetypes
        for (const signature& archetype : updateEntityTypes | std::views::values)
            updateNewSignatures.insert(archetype);
        for (const signature& archetype : archetypeSignatures)
            updateNewSignatures.erase(archetype);

        // create new archetype data holders
        for (const signature& archetype : updateNewSignatures)
        {
            updateNewType.clear();
            for (uint type = 0; type < NumTypes; ++type)
            {
                if (archetype[type])
                    updateNewType.push_back(type);
            }
            archetypeSignaturesInv[archetype] = static_cast<uint>(archetypeData.size());
            archetypeData.emplace_back(updateNewType);
            archetypeSignatures.push_back(archetype);
        }
        updateNewSignatures.clear();
        updateNewType.clear();

        // move entity to correct container and initialise data
        for (auto [entity, archetype] : updateEntityTypes)
        {
            const uint archetype_index = archetypeSignaturesInv[archetype];
            ArchetypeData& data = archetypeData[archetype_index];

            const EntityLocation old_location = entityLocations[entity];

            ArchetypeData& old_data = archetypeData[old_location.archetype];
            const signature& old_archetype = archetypeSignatures[old_location.archetype];

            std::vector<UpdateInstr>& updateQueue = entityUpdateQueue[entity];

            entityLocations[entity] = { archetype_index, static_cast<uint>(data.GetCount()) };

            data.IncCount(entity);

            signature updating {};

            // init all added components, destroy all removed components
            while (!updateQueue.empty())
            {
                const auto& [i, init_bytes] = updateQueue.back();
                updating[i] = true;

                if (!archetype[i])
                {
                    // destroy unused added components
                    if (init_bytes)
                        ComponentInfo.Destroy(init_bytes.get(), i);

                    updateQueue.pop_back();
                    continue;
                }

                // this type will be added, init with most recent update data
                archetype[i] = false;
                data.SetElem(i, init_bytes.get());

                updateQueue.pop_back();
            }

            // move or destroy all existing components
            for (uint i = 0; i < NumTypes; ++i)
                if (old_archetype[i])
                {
                    std::byte* elem = old_data.GetElem(old_location.index, i);
                    if (updating[i])
                        ComponentInfo.Destroy(elem, i);
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

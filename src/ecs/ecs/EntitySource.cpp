#include "EntitySource.h"

#include "Engine.h"

namespace ecs
{
    bool EntitySource::IsLocked(const TypeID type) const
    {
        return std::ranges::find(types, type) != types.end();
    }

    std::byte* EntitySource::TryGetFromSource(EngineMap& engineMap, const TypeID type) const
    {
        // there is no source
        if (!IsLocked(type) || !asset.IsValid())
            return nullptr;

        if (!engineMap.contains(asset))
        {
            // casually open an entire ecs
            Engine engine {};
            engine.AddFrom(asset, false);
            engine.Refresh();
            engineMap.insert_or_assign(asset, std::move(engine));
        }

        Engine& engine = engineMap[asset];
        const EntitySource& source = engine.entitySources[target];

        // this source has a source
        if (std::byte* data = source.TryGetFromSource(engineMap, type))
            return data;

        // this source has data
        auto [archetype, index] = engine.entities[target];
        return engine.archetypes[archetype].TryGetElem(index, type);
    }
}

#pragma once
#include <unordered_map>
#include <vector>

#include "ComponentConcepts.h"
#include "assets-system/AssetID.h"
#include "serialisation/array.h"

namespace ecs
{
    class Engine;

    struct EntitySource
    {
        using EngineMap = std::unordered_map<assets_system::AssetID, Engine>;

        EntityID target;
        assets_system::AssetID asset;
        std::vector<TypeID> types;

        bool IsLocked(TypeID type) const;

        std::byte* TryGetFromSource(EngineMap& engineMap, TypeID type) const;

        void Serialize(serial::Stream& m)
        {
            if (m.reading)
            {
                target = m.reader.Read<EntityID>();
                asset = m.reader.Read<assets_system::AssetID>();
            }
            else
            {
                m.writer.Write<EntityID>(target);
                m.writer.Write<assets_system::AssetID>(asset);
            }
            serial::SerializeVector(m, types);
        }
    };
}

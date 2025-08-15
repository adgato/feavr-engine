#pragma once
#include "Engine.h"

namespace ecs
{
    template <typename... Ts>
    void SerializeOnly(Engine& engine, serial::Stream& m)
    {
        const std::vector<TypeID> types { TypeRegistry::GetID<Ts>()... };
        const size_t count = TypeRegistry::RegisteredCount();
        std::string serialTypes {};
        for (TypeID i = 0; i < count; ++i)
        {
            serialTypes += TypeRegistry::GetInfo(i).name;
            if (i < count - 1)
                serialTypes += ", ";
        }

        if (m.reading)
            engine.ReadEngineTypes(serialTypes.c_str(), types, m);
        else
            engine.WriteEngineTypes(serialTypes.c_str(), types, m);
    }

    template <ComponentType... Ts>
    Entity NewAdd(Engine& engine, Ts&... data)
    {
        Entity e = engine.New();
        (engine.Add<Ts>(e, data), ...);
        return e;
    }

    template <ComponentType... Ts>
    std::tuple<Ts&...> Get(Engine& engine, Entity e)
    {
        return std::tuple<Ts&...>(engine.Get<Ts>(e)...);
    }

    void Widget(Engine& engine, Entity focus);
}

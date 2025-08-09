#pragma once
#include "Engine.h"

// serialize the component types given for the engine
#define ECS_SERIALIZE(engine, m, ...) \
    ecs::Serialize<__VA_ARGS__>(engine, #__VA_ARGS__, m)

namespace ecs
{
    template <typename... Ts>
    void Serialize(Engine& engine, const char* serialTypes, serial::Stream& m)
    {
        (TypeRegistry::Register<Ts>(), ...);
        const std::vector<TypeID> types { TypeRegistry::GetID<Ts>()... };
        if (m.reading)
        {
            engine.ReadEngineTypes(serialTypes, types, m);
            engine.Refresh();
        } else
        {
            engine.Refresh();
            engine.WriteEngineTypes(serialTypes, types, m);
        }
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
}

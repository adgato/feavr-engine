#pragma once
#include "ComponentTypeUniverse.h"

template <typename... Ts>
struct TypeUniverse
{
    static constexpr size_t size()
    {
        return sizeof...(Ts);
    }

    template <typename T>
    static constexpr size_t id()
    {
        size_t index = 0;
        ((std::is_same_v<T, Ts> ? true : (++index, false)) || ...);
        return index;
    }
};

using TypeID = const int*;
template <typename>
struct TypeIdentifier
{
    constexpr static int _id{};
    
    static constexpr TypeID id()
    {
        return &_id;
    }
};

template <typename T>
constexpr TypeID GetTypeID()
{
    return TypeIdentifier<T>::id();
}

namespace ecs
{
    inline constexpr size_t NumTypes = ComponentTypeUniverse::size();
    
    template <typename T>
    concept IsDestroyable = requires(T t)
    {
        { t.Destroy() } -> std::same_as<void>;
    };
    
    template <typename T, typename Universe>
    inline constexpr bool in_universe_v = false;

    template <typename T, typename... Ts>
    inline constexpr bool in_universe_v<T, TypeUniverse<Ts...>> = (std::same_as<T, Ts> || ...);

    template <typename T>
    concept ComponentType = std::is_trivially_copyable_v<T> && in_universe_v<T, ComponentTypeUniverse> && IsDestroyable<T>;

    using uint = uint32_t;
    using EntityID = uint32_t;
    using Entity = const EntityID;
    
    template <ComponentType T>
    constexpr uint GetComponentID()
    {
        return static_cast<uint>(ComponentTypeUniverse::id<T>());
    }
    
    struct TypeInfo
    {
        uint size;
        uint align;
    };
}

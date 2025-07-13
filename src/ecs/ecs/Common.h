#pragma once
#include <functional>
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

namespace ecs
{
    inline constexpr size_t NumTypes = ComponentTypeUniverse::size();

    struct TypeInfo
    {
        uint size;
        uint align;
    };

    template <typename T, typename Universe>
    inline constexpr bool in_universe_v = false;

    template <typename T, typename... Ts>
    inline constexpr bool in_universe_v<T, TypeUniverse<Ts...>> = (std::same_as<T, Ts> || ...);

    template <typename T>
    concept ComponentType =
            in_universe_v<T, ComponentTypeUniverse> && IsDestroyable<T> && IsSerialType<T>;

    template <typename>
    class UniverseInfo
    {
    public:
        // help code completion
        static TypeInfo& GetTypeInfo(uint)
        {
            static TypeInfo t;
            assert(false);
            return t;
        }

        static void Destroy(void*, uint) { assert(false); }
        static void Serialize(SerialManager*, void*, uint) { assert(false); }
    };

    template <ComponentType... Ts>
    class UniverseInfo<TypeUniverse<Ts...>>
    {
        std::array<TypeInfo, sizeof...(Ts)> typeInfo {};
        std::array<std::function<void(void*)>, sizeof...(Ts)> destroyers {};
        std::array<std::function<void(void*, SerialManager*)>, sizeof...(Ts)> serializers {};
        std::tuple<Ts...> defaults {};

        template <size_t I>
        void CopyDefaultRec(void* dest, const uint type) const
        {
            if (type == I)
            {
                const auto& value = std::get<I>(defaults);
                std::memcpy(dest, &value, sizeof(value));
                return;
            }
            if constexpr (I + 1 < sizeof...(Ts))
            {
                CopyDefaultRec<I + 1>(dest, type);
            }
        }

    public:
        UniverseInfo()
        {
            size_t i = 0;
            ((typeInfo[i++] = { sizeof(Ts), alignof(Ts) }), ...);
            i = 0;
            ((destroyers[i++] = [](void* t) { static_cast<Ts*>(t)->Destroy(); }), ...);
            i = 0;
            ((serializers[i++] = [](void* t, SerialManager* s) { static_cast<Ts*>(t)->Serialize(s); }), ...);
        }

        void CopyDefault(void* dest, const uint type) const
        {
            CopyDefaultRec<0>(dest, type);
        }

        const TypeInfo& GetTypeInfo(uint type) const
        {
            return typeInfo[type];
        }

        void Destroy(void* data, uint type) const
        {
            destroyers[type](data);
        }

        void Serialize(SerialManager* serialManager, void* data, uint type) const
        {
            serializers[type](data, serialManager);
        }
    };

    inline UniverseInfo<ComponentTypeUniverse> ComponentInfo {};
    using uint = uint32_t;
    using EntityID = uint32_t;
    using Entity = const EntityID;
    using signature = std::bitset<NumTypes>;

    template <ComponentType T>
    constexpr uint GetComponentID()
    {
        return static_cast<uint>(ComponentTypeUniverse::id<T>());
    }
}

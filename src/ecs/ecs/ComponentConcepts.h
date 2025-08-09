#pragma once
#include <cstdint>

#include "serialisation/SerialConcepts.h"

namespace ecs
{
    using TypeID = uint16_t;
    using uint = uint32_t;
    // should NOT be serialized directly (will break when two ECSs are unioned), use EntityRef for this instead
    using EntityID = uint32_t;
    using Entity = const EntityID;

    struct TypeInfo
    {
        TypeID type;
        size_t size;
        size_t align;
    };

    template <typename T>
    concept IsSerialTypeAndDestroyable = serial::IsDestroyable<T> && serial::IsSerialType<T>;

    template <typename T>
    concept ComponentType = std::is_trivially_copyable_v<T>;

    template<typename T, typename... Ts>
    inline constexpr bool one_of_v = std::disjunction_v<std::is_same<T, Ts>...>;

    template <typename T, typename... Ts>
    constexpr std::size_t index_of_type_v = []
    {
        std::size_t index = 0;
        (void)((std::is_same_v<T, Ts> ? true : (++index, false)) || ...);
        return index;
    }();
}

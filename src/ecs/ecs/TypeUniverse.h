#pragma once

#include <type_traits>

namespace ecs
{
    class SerialManager;

    using uint_s = uint16_t;

    template <typename T>
    concept IsDestroyable = requires(T t)
    {
        { t.Destroy() } -> std::same_as<void>;
    };

    template <typename T>
    concept IsSerialType = requires(T t, SerialManager* s)
    {
        { t.Serialize(s) } -> std::same_as<void>;
    } && std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>;

    template <typename T>
    concept Serializable = IsSerialType<T> || std::is_arithmetic_v<T>;

}

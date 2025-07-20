#pragma once

#include <cstdint>
#include <type_traits>

namespace serial
{
    class SerialManager;

    using TagID = uint8_t;
    using uint_s = uint16_t;

    template <typename T>
    concept IsDestroyable = requires(T t)
    {
        { t.Destroy() } -> std::same_as<void>;
    };

    template <typename T>
    concept IsSerialType = requires(T t, SerialManager& s)
    {
        { t.Serialize(s) } -> std::same_as<void>;
    } && std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>;

    template <typename T>
    concept IsSerializable = IsSerialType<T> || std::is_arithmetic_v<T>;

    template<TagID...>
    constexpr bool not_in_arr = true;

    template<TagID value, TagID head, TagID... tail>
    constexpr bool not_in_arr<value, head, tail...> = value != head && not_in_arr<value, tail...>;

    template<TagID...>
    constexpr bool is_distinct = true;

    template <TagID head, TagID... tail>
    constexpr bool is_distinct<head, tail...> = not_in_arr<head, tail...> && is_distinct<tail...>;
}

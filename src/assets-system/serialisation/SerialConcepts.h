#pragma once

#include <cstdint>
#include <type_traits>
#include <concepts>

namespace serial
{
    class Stream;

    using TagID = uint8_t;

    // expecting file size << 4GB
    using fsize = uint32_t;

    template <typename T>
    concept IsDestroyable = requires(T t)
    {
        { t.Destroy() } -> std::same_as<void>;
    };

    template <typename T>
    concept IsSerialType = requires(T t, Stream& s)
    {
        { t.Serialize(s) } -> std::same_as<void>;
    }  && std::is_default_constructible_v<T>;

    template <typename T>
    concept IsSerializable = IsSerialType<T> || std::is_trivially_copyable_v<T>;

    template<TagID...>
    constexpr bool is_ordered = true;

    template <TagID first, TagID second, TagID... tail>
    constexpr bool is_ordered<first, second, tail...> = first < second && is_ordered<second, tail...>;
}

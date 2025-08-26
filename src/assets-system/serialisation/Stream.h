#pragma once
#include <any>
#include <cassert>
#include <fmt/base.h>

#include "ReadByteStream.h"
#include "WriteByteStream.h"
#include "SerialConcepts.h"

#define SERIALIZABLE(tag, type) serial::Serializable<tag, type>

namespace serial
{
    template <TagID, IsSerializable T>
    struct Serializable
    {
        T value;

        operator T&() { return value; }
        operator const T&() const { return value; }

        T& operator*() { return value; }
        const T& operator*() const { return value; }

        T* operator->() { return &value; }
        const T* operator->() const { return &value; }

        void Serialize(Stream& m);
    };

    class Stream
    {
        bool MatchNextTag(TagID tag);

    public:
        std::any userData;
        bool reading = false;
        WriteByteStream writer;
        ReadByteStream reader;

        Stream() = default;

        ~Stream() { Destroy(); }

        Stream(const Stream& other) = delete;

        Stream(Stream&& other) noexcept = default;

        Stream& operator=(const Stream& other) = delete;

        Stream& operator=(Stream&& other) noexcept = delete;

        void InitRead()
        {
            reading = true;
            Destroy();
        }

        void InitWrite()
        {
            reading = false;
            Destroy();
            writer.Init();
        }

        void Destroy()
        {
            reader.Destroy();
            writer.Destroy();
        }

        // arguments should be in order of tag
        template <typename... Ts, TagID... tags> requires is_ordered<tags...>
        void SerializeComponent(Serializable<tags, Ts>&... fields)
        {
            ((MatchNextTag(tags) ? fields.Serialize(*this) : void()), ...);
            MatchNextTag(0xFF);
        }

        // neat, but inherently slow compile times
        // template <typename... Ts, TagID... tags> requires is_distinct<tags...>
        // void SerializeComponent(Serializable<tags, Ts>&... fields)
        // {
        //     constexpr size_t N = sizeof...(tags);
        //     constexpr std::array<TagID, N> tagArr { tags... };
        //     constexpr std::array<size_t, N> sorted = [&tagArr]() constexpr
        //     {
        //         std::array<size_t, N> idx {};
        //         std::iota(idx.begin(), idx.end(), 0);
        //         std::sort(idx.begin(), idx.end(), [&tagArr](size_t i, size_t j) { return tagArr[i] < tagArr[j]; });
        //         return idx;
        //     }();
        //
        //     auto fieldTuple = std::tie(fields...);
        //
        //     // Process fields in order of tag
        //     [&]<size_t... Is>(std::index_sequence<Is...>)
        //     {
        //         ((MatchNextTag(tagArr[sorted[Is]]) ? std::get<sorted[Is]>(fieldTuple).Serialize(*this) : void()), ...);
        //     }(std::make_index_sequence<N> {});
        //
        //     MatchNextTag(0xFF);
        // }

        template <typename T> requires std::is_trivially_copyable_v<T>
        void SerializeArray(T* data, const size_t size)
        {
            if (reading)
                reader.ReadArray<T>(data, size);
            else
                writer.WriteArray<T>(data, size);
        }
    };

    template <TagID tag, IsSerializable T>
    void Serializable<tag, T>::Serialize(Stream& m)
    {
        if constexpr (IsSerialType<T>)
        {
            size_t offset;
            size_t size = 0;
            if (m.reading)
            {
                size = m.reader.Read<fsize>();
                offset = m.reader.GetCount();
            } else
                offset = m.writer.Reserve<fsize>();

            value.Serialize(m);

            if (m.reading)
            {
                if (m.reader.GetCount() - offset != size)
                {
                    fmt::println("Warning: unexpected size serialized.");
                    m.reader.Jump(offset - m.reader.GetCount() + size);
                    value = {};
                }
            } else
                m.writer.WriteOver<fsize>(m.writer.GetCount() - offset - sizeof(fsize), offset);
        } else
        {
            if (m.reading)
            {
                const size_t size = m.reader.Read<fsize>();
                if (size == sizeof(T))
                    value = m.reader.Read<T>();
                else
                {
                    fmt::println("Warning: unexpected size serialized.");
                    m.reader.Jump(size);
                }
            } else
            {
                m.writer.Write<fsize>(sizeof(T));
                m.writer.Write<T>(value);
            }
        }
    }
}

#pragma once
#include <algorithm>
#include <cassert>
#include <numeric>
#include <ranges>
#include <fmt/core.h>

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

        void Serialize(SerialManager& m);
    };

    class SerialManager
    {
        bool MatchNextTag(const TagID tag)
        {
            if (!loading)
            {
                writer.Write<TagID>(tag);
                return true;
            }

            TagID readTag = reader.Read<TagID>();
            while (readTag < tag)
            {
                reader.Jump(reader.Read<uint_s>());
                readTag = reader.Read<TagID>();
            }
            if (readTag > tag)
                reader.Jump(-sizeof(TagID));

            return readTag == tag;
        }

    public:
        bool loading = false;
        WriteByteStream writer;
        ReadByteStream reader;

        void InitRead()
        {
            loading = true;
            Destroy();
        }

        void InitWrite()
        {
            loading = false;
            Destroy();
            writer.Init();
        }

        void Destroy()
        {
            reader.Destroy();
            writer.Destroy();
        }

        template <typename... Ts, TagID... tags> requires is_distinct<tags...>
        void SerializeComponent(Serializable<tags, Ts>&... fields)
        {
            constexpr size_t N = sizeof...(tags);
            constexpr std::array<TagID, N> tagArr { tags... };
            constexpr std::array<size_t, N> sorted = [&tagArr]() constexpr
            {
                std::array<size_t, N> idx {};
                std::iota(idx.begin(), idx.end(), 0);
                std::sort(idx.begin(), idx.end(), [&tagArr](size_t i, size_t j) { return tagArr[i] < tagArr[j]; });
                return idx;
            }();

            auto fieldTuple = std::tie(fields...);

            // Process fields in order of tag
            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                ((MatchNextTag(tagArr[sorted[Is]]) ? std::get<sorted[Is]>(fieldTuple).Serialize(*this) : void()), ...);
            }(std::make_index_sequence<N> {});

            MatchNextTag(0xFF);
        }

        template <typename T> requires std::is_trivially_copyable_v<T>
        void SerializeArray(T* data, const size_t size)
        {
            if (loading)
                reader.ReadArray<T>(data, size);
            else
                writer.WriteArray<T>(data, size);
        }
    };

    template <TagID tag, IsSerializable T>
    void Serializable<tag, T>::Serialize(SerialManager& m)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            if (m.loading)
            {
                const size_t size = m.reader.Read<uint_s>();
                if (size == sizeof(T))
                    value = m.reader.Read<T>();
                else
                {
                    fmt::println("Warning: unexpected size serialized.");
                    m.reader.Jump(size);
                }
            } else
            {
                m.writer.Write<uint_s>(sizeof(T));
                m.writer.Write<T>(value);
            }
        } else
        {
            size_t offset;
            size_t size = 0;
            if (m.loading)
            {
                size = m.reader.Read<uint_s>();
                offset = m.reader.GetCount();
            } else
                offset = m.writer.Reserve<uint_s>();

            value.Serialize(m);

            if (m.loading)
            {
                if (m.reader.GetCount() - offset != size)
                {
                    fmt::println("Warning: unexpected size serialized.");
                    m.reader.Jump(offset - m.reader.GetCount() + size);
                    value = {};
                }
            } else
                m.writer.WriteOver<uint_s>(m.writer.GetCount() - offset - sizeof(uint_s), offset);
        }
    }
}

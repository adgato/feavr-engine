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

        void Barrier(TagID tag, size_t& size, size_t& offset);

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

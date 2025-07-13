#pragma once
#include <bitset>
#include <vector>

#include "array.h"
#include "ByteStream.h"

using TypeID = const int*;

template <typename>
struct TypeIdentifier
{
    constexpr static int _id {};

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
    template <size_t N>
    std::vector<uint32_t> ReadTypeString(ReadByteStream& data, const char* targetTypes)
    {
        constexpr std::hash<std::string> stringHash {};

        std::vector<uint32_t> targetRemap {};
        std::array<size_t, N> targetHashes {};

        size_t c = 0;
        for (size_t i = 0; i < N; ++i)
        {
            std::string readType {};
            do readType += targetTypes[c++];
            while (readType.back() != ' ' && readType.back() != '\0');

            readType.pop_back();
            if (readType.back() == ',')
                readType.pop_back();

            targetHashes[i] = stringHash(readType);
        }

        std::vector<std::hash<std::string>> readHashes;
        while (true)
        {
            std::string readType {};
            do readType += data.Read<char>();
            while (readType.back() != ' ' && readType.back() != '\0');

            const bool end = readType.back() == '\0';

            readType.pop_back();
            if (readType.back() == ',')
                readType.pop_back();

            size_t readHash = stringHash(readType);
            for (size_t i = 0; i < N; ++i)
                if (readHash == targetHashes[i])
                {
                    targetRemap.push_back(i);
                    goto forbreak;
                }
            targetRemap.push_back(N);
        forbreak:

            if (end)
                break;
        }

        return targetRemap;
    }

    template <size_t N>
    std::vector<std::byte> WriteBitset(const std::vector<std::bitset<N>>& bits)
    {
        std::vector<std::byte> data((bits.size() * N + 7) / 8);
        size_t c = 0;
        for (size_t i = 0; i < bits.size(); ++i)
            for (size_t j = 0; j < N; ++j)
            {
                constexpr std::byte zero {};
                if (c % 8 == 0)
                    data[c / 8] = zero;
                data[c / 8] |= std::byte { static_cast<unsigned char>(bits[i][j] << (c % 8)) };
                c++;
            }
        return data;
    }

    template <size_t N>
    std::vector<std::bitset<N>> ReadBitset(ReadByteStream& data, size_t size, const std::vector<uint32_t>& remap)
    {
        std::vector<std::bitset<N>> bits(size);
        std::byte current { 0 };
        size_t c = 0;
        for (size_t i = 0; i < size; ++i)
            for (size_t j = 0; j < remap.size(); ++j)
            {
                constexpr std::byte zero {};
                if (c % 8 == 0)
                    current = data.Read<std::byte>();

                if (const uint32_t k = remap[j]; k < N)
                    bits[i][k] = (current & std::byte { static_cast<unsigned char>(1u << (c % 8)) }) > zero;
                c++;
            }
        return bits;
    }

    class SerialManager
    {
        bool loading = false;

        template <typename T> requires std::is_arithmetic_v<T>
        void Serialize(T* pValue)
        {
            if (loading)
                *pValue = reader.Read<T>();
            else
                writer.Write<T>(*pValue);
        }

    public:
        WriteByteStream writer;
        ReadByteStream reader;

        void SetModeRead()
        {
            loading = true;
        }

        void SetModeWrite()
        {
            loading = false;
            writer.Init();
        }

        void Reset()
        {
            reader.Destroy();
            writer.Destroy();
        }

        bool SkipToTag(const uint_s tag)
        {
            if (!loading)
                return true;

            uint_s readTag = reader.Read<uint_s>();
            while (readTag < tag)
            {
                reader.Skip(reader.Read<uint_s>());
                readTag = reader.Read<uint_s>();
            }
            if (readTag > tag)
                reader.Backup(sizeof(uint_s));

            return readTag == tag;
        }

        template <typename T> requires std::is_arithmetic_v<T>
        void SerializeVar(T* pValue, const uint_s tag)
        {
            if (loading)
            {
                if (!SkipToTag(tag))
                    return;
                const size_t size = reader.Read<uint_s>();
                if (size == sizeof(T))
                    *pValue = reader.Read<T>();
                else
                    reader.Skip(size);
            } else
            {
                writer.Write<uint_s>(tag);
                writer.Write<uint_s>(sizeof(T));
                writer.Write<T>(*pValue);
            }
        }

        template <IsSerialType T>
        void SerializeCmp(T* pValue, const uint_s tag)
        {
            size_t offset = 0;
            if (loading)
            {
                if (!SkipToTag(tag))
                    return;
                reader.Skip(sizeof(uint_s));
            } else
            {
                writer.Write<uint_s>(tag);
                offset = writer.Reserve<uint_s>();
            }
            pValue->Serialize();
            if (!loading)
                writer.WriteOver<uint_s>(writer.GetCount() - offset - sizeof(uint_s), offset);
        }

        template <Serializable T>
        void SerializeArr(array<T>* pValue, const uint_s tag)
        {
            size_t offset = 0;
            size_t count;
            if (loading)
            {
                if (!SkipToTag(tag))
                    return;
                const size_t size = reader.Read<uint_s>();
                count = reader.Read<uint_s>();
                if constexpr (std::is_arithmetic_v<T>)
                    if (size != count * sizeof(T) + sizeof(uint_s))
                    {
                        reader.Skip(size - sizeof(uint_s));
                        return;
                    }
                pValue->Init(count);
            } else
            {
                count = pValue->size();
                writer.Write<uint_s>(tag);
                offset = writer.Reserve<uint_s>();
                writer.Write<uint_s>(count);
            }
            T* data = pValue->data();
            for (size_t i = 0; i < count; ++i)
            {
                if constexpr (std::is_arithmetic_v<T>)
                    Serialize(data + i);
                else
                    (data + i)->Serialize();
            }
            if (!loading)
                writer.WriteOver<uint_s>(writer.GetCount() - offset - sizeof(uint_s), offset);
        }
    };
}

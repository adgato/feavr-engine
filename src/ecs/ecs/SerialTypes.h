#pragma once
#include <bitset>
#include <vector>

#include "serialisation/array.h"
#include "serialisation/ReadByteStream.h"
#include "serialisation/WriteByteStream.h"

using OldTypeID = const int*;

template <typename>
struct TypeIdentifier
{
    constexpr static int _id {};

    static constexpr OldTypeID id()
    {
        return &_id;
    }
};

template <typename T>
constexpr OldTypeID GetTypeID()
{
    return TypeIdentifier<T>::id();
}

namespace ecs
{
    template <size_t N>
    std::vector<uint32_t> ReadTypeString(serial::ReadByteStream& data, const char* targetTypes)
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
    std::vector<std::bitset<N>> ReadBitset(serial::ReadByteStream& data, size_t size, const std::vector<uint32_t>& remap)
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


}

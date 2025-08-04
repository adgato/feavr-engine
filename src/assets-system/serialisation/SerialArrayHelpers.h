#pragma once
#include <vector>

#include "Stream.h"

namespace serial
{
    template <IsSerializable T>
    void SerializeVector(Stream& m, std::vector<T>& array)
    {
        uint count = array.size();

        if (m.loading)
            count = m.reader.Read<uint>();
        else
            m.writer.Write<uint>(count);

        array.resize(count);
        T* ptr = array.data();

        if constexpr (std::is_arithmetic_v<T>)
            m.SerializeArray<T>(ptr, count);
        else
            for (uint i = 0; i < count; ++i)
                ptr[i].Serialize(m);
    }
}

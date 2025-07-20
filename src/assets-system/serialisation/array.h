#pragma once
#include "SerialConcepts.h"
#include <cassert>

#include "SerialManager.h"

namespace serial
{
    template <typename T>
    class array
    {
        T* ptr = nullptr;
        uint_s count = 0;

    public:
        array() = default;

        array(const T* data, const uint_s size)
        {
            Init(size);
            CopyFrom(data);
        }

        void Init(const uint_s size)
        {
            assert(static_cast<size_t>(size) * sizeof(T) <= 0xFFFF);
            Destroy();
            ptr = new T[size];
            count = size;
        }

        void Destroy() requires IsDestroyable<T> || std::is_arithmetic_v<T>
        {
            if (ptr)
            {
                if constexpr (IsDestroyable<T>)
                {
                    for (uint_s i = 0; i < count; ++i)
                        ptr[i].Destroy();
                }
                delete[] ptr;
            }
            ptr = nullptr;
            count = 0;
        }

        void CopyFrom(const T* other)
        {
            std::memcpy(ptr, other, count * sizeof(T));
        }

        T* data() { return ptr; }
        uint_s size() const { return count; }


        void Serialize(SerialManager& m) requires IsSerializable<T>
        {
            if (m.loading)
                Init(m.reader.Read<uint_s>());
            else
                m.writer.Write<uint_s>(count);

            if constexpr (std::is_arithmetic_v<T>)
                m.SerializeArray<T>(ptr, count);
            else
            {
                for (size_t i = 0; i < count; ++i)
                    (ptr + i)->Serialize();
            }
        }
    };
}

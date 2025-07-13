#pragma once
#include "TypeUniverse.h"

namespace ecs
{
    template <typename T> requires IsDestroyable<T> || std::is_arithmetic_v<T>
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
            ptr = new T[size];
            count = size;
        }

        void Destroy()
        {
            if constexpr (IsDestroyable<T>)
            {
                if (ptr)
                {
                    for (uint_s i = 0; i < count; ++i)
                        ptr[i].Destroy();
                }
            }
            if (ptr)
                delete[] ptr;
            ptr = nullptr;
            count = 0;
        }

        void CopyFrom(const T* other)
        {
            std::memcpy(ptr, other, count * sizeof(T));
        }

        T* data() { return ptr; }
        uint_s size() const { return count; }
    };

}

#pragma once
#include <cassert>
#include <utility>
#include <type_traits>
#include "Stream.h"

namespace serial
{
    template <IsSerializable T>
    class array
    {
        T* ptr = nullptr;
        uint count = 0;
        uint capacity = 0;

    public:
        array() = default;

        static array NewOfSize(const uint size)
        {
            array arr {};
            arr.Resize(size);
            return arr;
        }

        static array NewReserve(const uint capacity)
        {
            array arr {};
            arr.Reserve(capacity);
            return arr;
        }

        static array NewFromData(const T* data, const uint size)
        {
            array arr = NewOfSize(size);
            std::memcpy(arr.ptr, data, size * sizeof(T));
            return arr;
        }

    private:
        void IncreaseCapacity(const uint amount)
        {
            assert(capacity < UINT32_MAX - amount);

            capacity += amount;


            T* new_ptr = new T[capacity];

            if (ptr)
            {
                std::memcpy(new_ptr, ptr, count * sizeof(T));
                delete[] ptr;
            }

            ptr = new_ptr;
        }

    public:
        void Destroy()
        {
            if (!ptr)
                return;

            if constexpr (IsDestroyable<T>)
            {
                for (uint i = 0; i < count; ++i)
                    ptr[i].Destroy();
            }
            delete[] ptr;

            ptr = nullptr;
            count = 0;
            capacity = 0;
        }

        void Resize(const uint count)
        {
            this->capacity = 0;
            IncreaseCapacity(count);
            this->count = count;
        }

        void Reserve(const uint capacity)
        {
            this->capacity = 0;
            IncreaseCapacity(capacity);
        }

        void push_back(const T& elem)
        {
            if (count >= capacity)
            {
                IncreaseCapacity(std::max(count, 64u) >> 1);
                assert(count < capacity);
            }
            ptr[count++] = elem;
        }

        void push_back(T&& elem)
        {
            if (count >= capacity)
            {
                IncreaseCapacity(std::max(count, 64u) >> 1);
                assert(count < capacity);
            }
            ptr[count++] = std::move(elem);
        }

        T* data()
        {
            assert(ptr);
            return ptr;
        }

        const T* data() const
        {
            assert(ptr);
            return ptr;
        }

        uint size() const { return count; }

        void Serialize(Stream& m)
        {
            if (m.loading)
                Resize(m.reader.Read<uint>());
            else
                m.writer.Write<uint>(count);

            if constexpr (std::is_arithmetic_v<T>)
                m.SerializeArray<T>(ptr, count);
            else
                for (uint i = 0; i < count; ++i)
                    ptr[i].Serialize(m);
        }
    };
}

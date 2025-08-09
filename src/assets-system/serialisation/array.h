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
        fsize count = 0;
        fsize capacity = 0;

    public:
        array() = default;

        static array NewOfSize(const fsize size)
        {
            array arr {};
            arr.Resize(size);
            return arr;
        }

        static array NewReserve(const fsize capacity)
        {
            array arr {};
            arr.Reserve(capacity);
            return arr;
        }

        static array NewFromData(const T* data, const fsize size)
        {
            array arr = NewOfSize(size);
            std::memcpy(arr.ptr, data, size * sizeof(T));
            return arr;
        }

    private:
        void IncreaseCapacity(const fsize amount)
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
                for (fsize i = 0; i < count; ++i)
                    ptr[i].Destroy();
            }
            delete[] ptr;

            ptr = nullptr;
            count = 0;
            capacity = 0;
        }

        void Resize(const fsize count)
        {
            this->capacity = 0;
            IncreaseCapacity(count);
            this->count = count;
        }

        void Reserve(const fsize capacity)
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

        fsize size() const { return count; }

        void Serialize(Stream& m)
        {
            if (m.reading)
                Resize(m.reader.Read<fsize>());
            else
                m.writer.Write<fsize>(count);

            if constexpr (std::is_arithmetic_v<T>)
                m.SerializeArray<T>(ptr, count);
            else
                for (fsize i = 0; i < count; ++i)
                    ptr[i].Serialize(m);
        }
    };
}

#pragma once
#include <cassert>
#include <utility>
#include <type_traits>
#include "Stream.h"

namespace serial
{
    template <IsSerializable T> requires std::is_trivially_copyable_v<T>
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
            assert(capacity > count);

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
            if (count > this->count)
            {
                capacity = 0;
                IncreaseCapacity(count);
            }
            this->count = count;
        }

        void Reserve(const fsize capacity)
        {
            if (capacity > this->capacity)
            {
                this->capacity = 0;
                IncreaseCapacity(capacity);
            }
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
            return ptr;
        }

        const T* data() const
        {
            return ptr;
        }

        fsize size() const { return count; }

        void Serialize(Stream& m)
        {
            if (m.reading)
                Resize(m.reader.Read<fsize>());
            else
                m.writer.Write<fsize>(count);

            if constexpr (IsSerialType<T>)
                for (fsize i = 0; i < count; ++i)
                    ptr[i].Serialize(m);
            else
                m.SerializeArray<T>(ptr, count);
        }
    };

    template <typename T> requires std::is_trivially_copyable_v<T>
    void SerializeVector(Stream& m, std::vector<T>& vector)
    {
        array<T> serial {};
        if (m.reading)
        {
            serial.Serialize(m);
            vector.resize(serial.size());
            std::memcpy(vector.data(), serial.data(), vector.size() * sizeof(T));
        }
        else
        {
            serial = array<T>::NewFromData(vector.data(), vector.size());
            serial.Serialize(m);
        }
        serial.Destroy();
    }
}

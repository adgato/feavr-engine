#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <span>
#include <string>
#include <type_traits>

namespace serial
{
    class WriteByteStream
    {
        std::byte* data = nullptr;
        size_t count = 0;
        size_t capacity = 1;

    public:
        void Init();

        void Destroy();

        void SaveToFile(const char* filePath) const;
        std::span<std::byte> AsSpan() const { return std::span(data, count); };

        size_t GetCount() const { return count; }

        template <typename T>
        size_t Reserve()
        {
            const size_t index = count;
            count += sizeof(T);
            return index;
        }

        template <typename T> requires std::is_trivially_copyable_v<T>
        void Write(const T& value)
        {
            if (count + sizeof(T) > capacity)
            {
                capacity = count + sizeof(T);
                capacity += capacity >> 1;

                data = static_cast<std::byte*>(std::realloc(data, capacity));
                if (!data)
                    throw std::bad_alloc();
            }
            std::memcpy(data + count, &value, sizeof(T));
            count += sizeof(T);
        }

        template <typename T> requires std::is_trivially_copyable_v<T>
        void WriteOver(const T& value, const size_t offset)
        {
            std::memcpy(data + offset, &value, sizeof(T));
        }

        void WriteString(const std::string& value)
        {
            WriteArray<char>(value.c_str(), value.size() + 1);
        }

        template <typename T> requires std::is_trivially_copyable_v<T>
        void WriteArray(const T* values, const size_t size)
        {
            if (count + sizeof(T) * size > capacity)
            {
                capacity = count + sizeof(T) * size;
                capacity += capacity >> 1;

                data = static_cast<std::byte*>(std::realloc(data, capacity));
                if (!data)
                    throw std::bad_alloc();
            }
            std::memcpy(data + count, values, sizeof(T) * size);
            count += sizeof(T) * size;
        }
    };
}

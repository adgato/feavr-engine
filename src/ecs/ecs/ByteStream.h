#pragma once

#include <fstream>
#include <cassert>
#include <cstring>
#include <type_traits>

class ReadByteStream
{
    std::byte* data = nullptr;
    size_t count = 0;

public:
    void LoadFromFile(const char* filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        assert(file.is_open() && "Could not open file");

        const size_t fileSize = file.tellg();

        data = static_cast<std::byte*>(malloc(fileSize));
        if (!data)
            throw std::bad_alloc();
        count = 0;

        file.seekg(0);
        file.read(reinterpret_cast<char*>(data), fileSize);
        file.close();
    }

    void Destroy()
    {
        if (data)
            free(data);
        data = nullptr;
    }

    size_t GetCount() const { return count; }

    template <typename T> requires std::is_trivially_copyable_v<T>
    T Read()
    {
        T value;
        std::memcpy(&value, data + count, sizeof(T));
        count += sizeof(T);
        return value;
    }

    void Skip(const size_t size)
    {
        count += size;
    }

    void Backup(const size_t size)
    {
        count -= size;
    }
};

class WriteByteStream
{
    std::byte* data = nullptr;
    size_t count = 0;
    size_t capacity = 1;

public:
    void Init()
    {
        if (data)
            free(data);
        count = 0;
        capacity = 1024;
        data = static_cast<std::byte*>(malloc(capacity));
        if (!data)
            throw std::bad_alloc();
    }

    void Destroy()
    {
        if (data)
            free(data);
        data = nullptr;
    }

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
            while (count + sizeof(T) > capacity)
                capacity *= 2;

            data = static_cast<std::byte*>(realloc(data, capacity));
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

    template <typename T> requires std::is_trivially_copyable_v<T>
    void WriteArray(const T* values, const size_t size)
    {
        if (count + sizeof(T) * size > capacity)
        {
            while (count + sizeof(T) * size > capacity)
                capacity *= 2;

            data = static_cast<std::byte*>(realloc(data, capacity));
            if (!data)
                throw std::bad_alloc();
        }
        std::memcpy(data + count, values, sizeof(T) * size);
        count += sizeof(T) * size;
    }

    void SaveToFile(const char* filePath) const
    {
        std::ofstream file(filePath, std::ios::binary);

        assert(file.is_open() && "Could not open file");

        if (data == nullptr)
        {
            file.close();
            return;
        }

        file.write(reinterpret_cast<const char*>(data), count);
        file.close();
    }
};
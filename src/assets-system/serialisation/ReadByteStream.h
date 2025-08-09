#pragma once

#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <type_traits>

namespace serial
{
    class ReadByteStream
    {
        std::byte* data = nullptr;
        size_t count = 0;
        bool needsDestroy = true;

    public:
        void LoadFromFile(const char* filePath);

        // copies span data into reader
        void CopyFrom(const std::span<const std::byte>& span);

        // does not copy span data into reader, make sure data sticks around until destroy
        void ViewFrom(const std::span<std::byte>& span);

        void Destroy();

        size_t GetCount() const { return count; }

        void Jump(const long size) { count += size; }
        void JumpAbs(const long size) { count = size; }

        template <typename T> requires std::is_trivially_copyable_v<T>
        T Read()
        {
            T value;
            std::memcpy(&value, data + count, sizeof(T));
            count += sizeof(T);
            return value;
        }

        std::string ReadString()
        {
            std::string result {};
            do result += Read<char>();
            while (result.back() != '\0');
            return result;
        }

        template <typename T> requires std::is_trivially_copyable_v<T>
        void ReadArray(T* dest, const size_t size)
        {
            std::memcpy(dest, data + count, sizeof(T) * size);
            count += sizeof(T) * size;
        }
    };
}

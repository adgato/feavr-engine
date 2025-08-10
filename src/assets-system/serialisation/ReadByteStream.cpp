#include "ReadByteStream.h"
#include "lz4.h"
#include <fstream>
#include <cstddef>

namespace serial
{
    void ReadByteStream::LoadFromFile(const char* filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            throw std::ios::failure("Could not open file");

        const size_t fileSize = file.tellg();

        data = static_cast<std::byte*>(std::malloc(fileSize));
        needsDestroy = true;
        if (!data)
            throw std::bad_alloc();
        count = 0;

        file.seekg(0);
        file.read(reinterpret_cast<char*>(data), fileSize);
        file.close();
    }

    void ReadByteStream::CopyFrom(const std::span<const std::byte>& span)
    {
        data = static_cast<std::byte*>(std::malloc(span.size_bytes()));
        needsDestroy = true;
        if (!data)
            throw std::bad_alloc();

        std::memcpy(data, span.data(), span.size_bytes());
        count = 0;
    }

    void ReadByteStream::ViewFrom(const std::span<std::byte>& span)
    {
        data = span.data();
        needsDestroy = false;
    }

    void ReadByteStream::Destroy()
    {
        if (needsDestroy && data)
            std::free(data);
        data = nullptr;
    }
}

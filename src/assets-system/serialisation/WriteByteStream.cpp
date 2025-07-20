#include "WriteByteStream.h"

#include <fstream>

namespace serial
{
    void WriteByteStream::Init()
    {
        if (data)
            std::free(data);
        count = 0;
        capacity = 1024;
        data = static_cast<std::byte*>(std::malloc(capacity));
        if (!data)
            throw std::bad_alloc();
    }

    void WriteByteStream::Destroy()
    {
        if (data)
            std::free(data);
        data = nullptr;
    }

    void WriteByteStream::SaveToFile(const char* filePath) const
    {
        std::ofstream file(filePath, std::ios::binary);

        if (!file.is_open())
            throw std::ios::failure("Could not open file");

        if (data == nullptr)
        {
            file.close();
            return;
        }

        file.write(reinterpret_cast<const char*>(data), count);
        file.close();
    }
}

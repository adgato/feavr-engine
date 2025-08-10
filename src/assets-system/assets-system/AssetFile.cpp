#include "AssetFile.h"

#include <fstream>
#include <lz4.h>

#include "serialisation/Stream.h"

namespace assets_system
{
    void AssetFile::Serialize(serial::Stream& m)
    {
        uint32_t blobSize = 0;
        if (m.reading)
        {
            const uint32_t typeInt = m.reader.Read<uint32_t>();
            version = m.reader.Read<uint32_t>();
            blobSize = m.reader.Read<uint32_t>();

            std::memcpy(type, &typeInt, sizeof(uint32_t));
            blob.resize(blobSize);
        } else
        {
            blobSize = blob.size();

            uint32_t typeInt;
            std::memcpy(&typeInt, type, sizeof(uint32_t));

            m.writer.Write<uint32_t>(typeInt);
            m.writer.Write<uint32_t>(version);
            m.writer.Write<uint32_t>(blobSize);
        }
        header.Serialize(m);
        m.SerializeArray(blob.data(), blobSize);
    }

    void AssetFile::Save(const char* filePath)
    {
        serial::Stream m {};

        m.InitWrite();
        Serialize(m);
        m.writer.SaveToFile(filePath);
    }

    serial::Stream AssetFile::ReadFromBlob(const size_t offset /* = 0 */, const size_t size /* = ~0ul */) const
    {
        serial::Stream m {};
        m.InitRead();

        if (const uint64_t* decompressedSize = header.GetIfInt("Decompressed Size"))
        {
            char* dest = static_cast<char*>(std::malloc(*decompressedSize));

            const int writtenSize = LZ4_decompress_safe(reinterpret_cast<const char*>(blob.data()), dest, blob.size(), *decompressedSize);
            assert(writtenSize > 0);
            m.reader.CopyFrom(std::span(reinterpret_cast<std::byte*>(dest) + offset, std::min(static_cast<size_t>(writtenSize), size)));

            std::free(dest);
        } else
            m.reader.CopyFrom(std::span(blob.data() + offset, std::min(blob.size(), size)));

        return m;
    }

    void AssetFile::WriteToBlob(const serial::Stream& m, const bool compress /* = false*/)
    {
        assert(!m.reading);
        const std::span<std::byte> span = m.writer.AsSpan();

        if (compress)
        {
            const int bound = LZ4_compressBound(span.size());
            blob.resize(bound);
            const int size = LZ4_compress_default(reinterpret_cast<const char*>(span.data()), reinterpret_cast<char*>(blob.data()), span.size(), blob.size());
            assert(size > 0);
            blob.resize(size);
            header["Decompressed Size"] = span.size();
        } else
        {
            blob.resize(span.size());
            std::memcpy(blob.data(), span.data(), span.size());
        }
    }

    AssetFile AssetFile::Load(const char* filePath)
    {
        serial::Stream m {};
        AssetFile assetFile {};

        m.InitRead();

        try
        {
            m.reader.LoadFromFile(filePath);
            assetFile.Serialize(m);
        } catch (std::exception)
        {
            fmt::println(stderr, "Unknown asset file: {}", filePath);
            return Invalid();
        }

        return assetFile;
    }
}

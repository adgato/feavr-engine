#include "AssetFile.h"

#include <fstream>

#include "serialisation/Stream.h"

namespace assets_system
{
    void AssetFile::Serialize(serial::Stream& m)
    {
        uint32_t blobSize = 0;
        if (m.loading)
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

    serial::Stream AssetFile::ReadFromBlob()
    {
        serial::Stream m {};
        m.InitRead();
        m.reader.LoadFromSpan(std::span(blob.data(), blob.size()));
        return m;
    }

    void AssetFile::WriteToBlob(const serial::Stream& m)
    {
        assert(!m.loading);
        const std::span<std::byte> span = m.writer.AsSpan();

        blob.resize(span.size());
        std::memcpy(blob.data(), span.data(), span.size());
    }

    AssetFile AssetFile::Load(const char* filePath)
    {
        serial::Stream m {};
        AssetFile assetFile {};

        m.InitRead();
        m.reader.LoadFromFile(filePath);
        assetFile.Serialize(m);

        return assetFile;
    }
}

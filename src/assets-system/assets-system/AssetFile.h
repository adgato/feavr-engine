#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

#include "serialisation/simple_json.h"

namespace assets_system
{
    class AssetFile
    {
    public:
        char type[4];
        uint32_t version;
        serial::simple_json header {};
        std::vector<std::byte> blob {};

    private:
        AssetFile() = default;

    public:
        AssetFile(const char type[4], const uint32_t version) : version(version)
        {
            std::memcpy(this->type, type, 4);
        }

        void Serialize(serial::Stream& m);

        void Save(const char* filePath);

        serial::Stream ReadFromBlob(size_t offset = 0, size_t size = ~0ul) const;
        void WriteToBlob(const serial::Stream& m);

        static AssetFile Load(const char* filePath);

        static AssetFile Invalid()
        {
            static AssetFile empty;
            return empty;
        }

        bool HasFormat(const char expectedType[4], const uint32_t expectedVersion) const
        {
            return
                    type[0] == expectedType[0] &&
                    type[1] == expectedType[1] &&
                    type[2] == expectedType[2] &&
                    type[3] == expectedType[3] && version == expectedVersion;
        }
    };
}

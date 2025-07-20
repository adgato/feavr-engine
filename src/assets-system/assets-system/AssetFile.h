#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace assets_system
{
    class AssetFile
    {
    public:
        char type[4];
        uint32_t version;
        std::string json = "";
        std::vector<std::byte> blob {};

    private:
        AssetFile() = default;

    public:
        AssetFile(const char* type, const uint32_t version) : version(version)
        {
            std::memcpy(this->type, type, 4);
        }

        void Save(const char* filePath) const;
        static AssetFile Load(const char* filePath);

        static const AssetFile& Invalid()
        {
            static AssetFile empty;
            return empty;
        }

        bool IsValid() const { return type[0] != '\0' && type[1] != '\0' && type[2] != '\0' && type[3] != '\0'; }
    };
}

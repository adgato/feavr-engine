#include "AssetFile.h"

#include <fstream>

namespace assets_system
{
    void AssetFile::Save(const char* filePath) const
    {
        std::ofstream file(filePath, std::ios::binary);

        if (!file.is_open())
            throw std::ios::failure("Could not open file");

        const uint32_t jsonlength = json.size();
        const uint32_t bloblength = blob.size();

        file.write(type, 4);
        file.write(reinterpret_cast<const char*>(&version), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&jsonlength), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&bloblength), sizeof(uint32_t));
        file.write(json.data(), jsonlength);
        file.write(reinterpret_cast<const char*>(blob.data()), bloblength);
    }

    AssetFile AssetFile::Load(const char* filePath)
    {
        AssetFile assetFile;
        std::ifstream file(filePath, std::ios::binary);

        if (!file.is_open())
            throw std::ios::failure("Could not open file");

        uint32_t jsonlength = 0;
        uint32_t bloblength = 0;

        file.read(assetFile.type, 4);
        file.read(reinterpret_cast<char*>(&assetFile.version), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&jsonlength), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&bloblength), sizeof(uint32_t));

        assetFile.json.resize(jsonlength);
        assetFile.blob.resize(bloblength);

        file.read(assetFile.json.data(), jsonlength);
        file.read(reinterpret_cast<char*>(assetFile.blob.data()), bloblength);

        return assetFile;
    }
}

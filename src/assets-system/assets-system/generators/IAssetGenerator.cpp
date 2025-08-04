#include "IAssetGenerator.h"

#include "assets-system/AssetManager.h"

namespace assets_system
{
    void IAssetGenerator::AddExtensions(const char* assetType, const std::initializer_list<const char*>& extensions)
    {
        for (const char* extension : extensions)
            AssetManager::AddAssetGenerator(extension, assetType, this);
    }
}

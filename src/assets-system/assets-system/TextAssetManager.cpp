#include "TextAssetManager.h"

#include "AssetFile.h"
#include "AssetManager.h"

namespace assets_system
{
    std::vector<std::string> TextAssetManager::GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents)
    {
        const std::string textFileName = assetPath + ".asset";
        const std::string fullPath = GEN_ASSET_DIR + textFileName;

        AssetFile textAsset("TEXT", 0);
        textAsset.json = "";
        textAsset.blob = std::move(contents);

        textAsset.Save(fullPath.c_str());

        return { textFileName };
    }
}

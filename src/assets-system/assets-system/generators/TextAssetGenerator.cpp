#include "StandardAssetGenerators.h"
#include "assets-system/AssetFile.h"
#include "assets-system/AssetManager.h"

std::vector<std::string> TextAssetGenerator::GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents)
{
    const std::string textFileName = assetPath + ".asset";
    const std::string fullPath = assets_system::GEN_ASSET_DIR + textFileName;

    assets_system::AssetFile textAsset("TEXT", 0);
    textAsset.blob = std::move(contents);

    textAsset.Save(fullPath.c_str());

    return { textFileName };
}
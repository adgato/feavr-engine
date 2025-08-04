#include "StandardAssetGenerators.h"
#include "assets-system/AssetManager.h"
#include "serialisation/Stream.h"

std::vector<std::string> AssetAssetGenerator::GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents)
{
    // just copy it over
    const std::string absPath = assets_system::GEN_ASSET_DIR + assetPath;
    serial::Stream m {};
    m.InitWrite();
    m.writer.WriteArray(contents.data(), contents.size());
    m.writer.SaveToFile(absPath.c_str());
    return { assetPath };
}

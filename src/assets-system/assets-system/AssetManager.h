#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "AssetFile.h"
#include "AssetID.h"

namespace assets_system
{
    constexpr auto GEN_ASSET_DIR = PROJECT_ROOT"/assets-meta/generated/";

    class AssetManager
    {
        enum AssetColumns { ID = 0, PATH = 1, DIRTY = 2 };

        static std::vector<std::string> GenerateAsset(const std::string& assetPath);
        static void WriteAssetLookup(const std::unordered_map<uint32_t, std::vector<std::string>>& assetNames);

    public:
        static bool RefreshAssets(bool refreshAll = false);

        static AssetFile LoadAsset(AssetID assetId);

        static std::string PrettyNameOfAsset(const std::string& relativeAssetPath);
    };
}

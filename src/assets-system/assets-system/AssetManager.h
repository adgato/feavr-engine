#pragma once

#include "AssetFile.h"
#include "AssetID.h"

namespace assets_system
{
    class IAssetGenerator;
    constexpr auto GEN_ASSET_DIR = PROJECT_ROOT"/assets-meta/generated/";
    constexpr auto ASSET_DIR = PROJECT_ROOT"/assets/";

    class AssetManager
    {
        enum AssetColumns { ID = 0, PATH = 1, DIRTY = 2 };

        static std::vector<std::string> GenerateAsset(const std::string& assetPath);
        static void WriteAssetLookup(const std::unordered_map<uint32_t, std::vector<std::string>>& assetNames);

        inline static std::unordered_map<size_t, IAssetGenerator*> assetGenerators {};
        inline static std::unordered_map<size_t, const char*> assetTypes {};
    public:

        static void AddAssetGenerator(const char* extension, const char* assetType, IAssetGenerator* delegate);

        static bool RefreshAssets(bool refreshAll = false);

        static AssetFile LoadAsset(AssetID assetId);

        static std::string PrettyNameOfAsset(const std::string& relativeAssetPath);
    };
}

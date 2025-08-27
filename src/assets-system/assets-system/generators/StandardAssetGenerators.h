#pragma once
#include "IAssetGenerator.h"

MAKE_ASSET_GENERATOR("SHAD", ShaderAssetGenerator, ".hlsl");
MAKE_ASSET_GENERATOR("TEXT", TextAssetGenerator, ".txt");

class AssetAssetGenerator final : public assets_system::IAssetGenerator
{
public:
    static void Register()
    {
        static AssetAssetGenerator instance {};
        instance.AddExtensions("ASET", { ".asset" });
    }

    std::vector<std::string> GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents) override;
};
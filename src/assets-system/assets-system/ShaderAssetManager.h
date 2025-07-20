#pragma once
#include <string>
#include <vector>

namespace assets_system
{
    class ShaderAssetManager
    {
    public:
        static std::vector<std::string> GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents);
    };
}

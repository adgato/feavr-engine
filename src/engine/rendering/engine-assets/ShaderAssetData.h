#pragma once
#include "assets-system/AssetID.h"

namespace engine_assets
{
    using namespace assets_system;
    struct ShaderAssetData
    {
        VkShaderModule module;
        std::string entry;
        VkShaderStageFlagBits stage;
        VkDevice device;

        void Destroy();

        static ShaderAssetData Load(VkDevice device, AssetID asset);
    };
}

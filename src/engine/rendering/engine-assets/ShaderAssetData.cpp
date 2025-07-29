#include "ShaderAssetData.h"

#include "assets-system/AssetManager.h"
#include "rendering/utility/VulkanCheck.h"

namespace engine_assets
{
    void ShaderAssetData::Destroy()
    {
        vkDestroyShaderModule(device, module, nullptr);
        module = nullptr;
    }

    ShaderAssetData ShaderAssetData::Load(const VkDevice device, const AssetID asset)
    {
        AssetFile assetFile = AssetManager::LoadAsset(asset);
        assert(assetFile.HasFormat("SHAD", 0));

        ShaderAssetData data;
        data.device = device;
        data.entry = (std::string)assetFile.header["entry"];

        const auto profile = (std::string)assetFile.header["profile"];
        if ("vs" == profile) data.stage = VK_SHADER_STAGE_VERTEX_BIT;
        else if ("ps" == profile) data.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        else if ("gs" == profile) data.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        else if ("cs" == profile) data.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        else assert(false && "Profile not recognised.");

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;

        createInfo.codeSize = assetFile.blob.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(assetFile.blob.data());

        VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &data.module));

        return data;
    }
}

#pragma once
#include <string>
#include <vector>

#define MAKE_ASSET_GENERATOR(AssetType, TypeName, ...) \
    class TypeName final : public assets_system::IAssetGenerator \
    { \
    public: \
        static void Register() \
        { \
            static TypeName instance {}; \
            instance.AddExtensions(AssetType, { __VA_ARGS__ }); \
        } \
        std::vector<std::string> GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents) override; \
    }; \

namespace assets_system
{
    class IAssetGenerator
    {
    public:
        void AddExtensions(const char* assetType, const std::initializer_list<const char*>& extensions);

        virtual ~IAssetGenerator() = default;

        virtual std::vector<std::string> GenerateAssets(const std::string& assetPath, std::vector<std::byte>&& contents) = 0;
    };
}


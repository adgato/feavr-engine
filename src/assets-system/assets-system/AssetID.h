#pragma once

namespace assets_system
{
    struct AssetID
    {
        // identifier of asset
        uint32_t id = ~0u;
        // index of file generated from asset
        uint32_t idx = ~0u;

        bool IsValid() const
        {
            return id < ~0u && idx < ~0u;
        }

        static AssetID Invalid()
        {
            return { ~0u, ~0u };
        }

        bool operator==(const AssetID& other) const
        {
            return id == other.id && idx == other.idx;
        }
    };
}

template <>
struct std::hash<assets_system::AssetID>
{
    size_t operator()(const assets_system::AssetID& asset) const noexcept
    {
        return static_cast<size_t>(asset.id) << 32 | static_cast<size_t>(asset.idx);
    }
};

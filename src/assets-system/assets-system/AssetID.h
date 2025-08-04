#pragma once

namespace assets_system
{
    struct AssetID
    {
        // identifier of asset
        uint32_t id;
        // index of file generated from asset
        uint32_t idx;

        bool IsInvalid() const
        {
            return id == ~0u && idx == ~0u;
        }

        static AssetID Invalid()
        {
            return { ~0u, ~0u };
        }
    };
}

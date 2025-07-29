#pragma once
#include <cstdint>

#include "serialisation/Stream.h"

namespace assets_system
{
    struct AssetID
    {
        // identifier of asset
        SERIALIZABLE(0, uint32_t) id;
        // index of file generated from asset
        SERIALIZABLE(1, uint32_t) idx;

        void Serialize(serial::Stream& m)
        {
            m.SerializeComponent(id, idx);
        }
        static void Destroy() {}
    };

}

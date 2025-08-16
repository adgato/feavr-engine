#pragma once
#include "serialisation/Stream.h"

namespace rendering
{
    class PassSystem;

    struct SubMesh
    {
        SERIALIZABLE(0, uint32_t) meshIndex;
        SERIALIZABLE(1, uint32_t) firstIndex;
        SERIALIZABLE(2, uint32_t) indexCount;
        PassSystem* passMeshManager;

        void Serialize(serial::Stream& m)
        {
            m.SerializeComponent(meshIndex, firstIndex, indexCount);
        }

        void Destroy();
    };
}

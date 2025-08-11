#pragma once
#include "ecs/ComponentConcepts.h"
#include "serialisation/array.h"

namespace rendering::passes
{
    template <typename>
    struct PassComponent
    {
        SERIALIZABLE(0, uint32_t) passGroup;
        SERIALIZABLE(1, serial::array<ecs::EntityID>) transforms;

        void Serialize(serial::Stream& m)
        {
            m.SerializeComponent(passGroup, transforms);
        }

        void Destroy()
        {
            transforms->Destroy();
        }
    };
}

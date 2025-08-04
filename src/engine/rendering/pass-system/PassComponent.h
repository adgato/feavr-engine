#pragma once
#include "ecs/SerialEntity.h"
#include "serialisation/array.h"

namespace rendering::passes
{
    template <typename>
    struct PassComponent
    {
        SERIALIZABLE(0, uint32_t) passGroup;

        // entites from another ECS, we have to manually offset these when unioning
        SERIALIZABLE(1, serial::array<ecs::EntityID>) entities;

        void Serialize(serial::Stream& m)
        {
            m.SerializeComponent(passGroup, entities);
        }

        void Destroy()
        {
            entities->Destroy();
        }
    };
}

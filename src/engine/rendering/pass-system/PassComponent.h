#pragma once
#include "../../../ecs/ecs/EntityRef.h"
#include "serialisation/array.h"

template <typename>
struct PassComponent
{
    SERIALIZABLE(0, uint32_t) passGroup;
    SERIALIZABLE(1, serial::array<ecs::EntityRef>) transforms;

    void Serialize(serial::Stream& m)
    {
        m.SerializeComponent(passGroup, transforms);
    }

    void Destroy()
    {
        transforms->Destroy();
    }
};
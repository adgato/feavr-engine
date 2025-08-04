#pragma once
#include "ComponentConcepts.h"
#include "serialisation/Stream.h"

namespace ecs
{
    struct EntityRef
    {
        EntityID localID;
        void Serialize(serial::Stream& m)
        {
            m.SerializeEntity(localID);
        }
    };
}

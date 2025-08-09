#pragma once
#include "ComponentConcepts.h"
#include "serialisation/Stream.h"

namespace ecs
{
    struct EntityRef
    {
        EntityID id;

        void Serialize(serial::Stream& m)
        {
            // userData is expected to be an offset for the id. this is needed to automatically update the EntityRef when engines are unioned.
            if (m.reading)
                id = m.reader.Read<EntityID>() + std::any_cast<EntityID>(m.userData);
            else
                m.writer.Write<EntityID>(id);
        }
    };
}

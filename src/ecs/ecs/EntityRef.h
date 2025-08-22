#pragma once
#include "ecs/ComponentConcepts.h"
#include "ecs/TypeRegistry.h"
#include "serialisation/Stream.h"

namespace ecs
{
    struct EntityRef
    {
        EntityID id = BadMaxEntity;

        EntityRef() = default;
        EntityRef(Entity id) : id(id) {}
        operator Entity() const { return id; }

        void Serialize(serial::Stream& m)
        {
            // userData is expected to have an offset for the id. this is needed to automatically update the EntityRef when engines are unioned.
            if (m.reading)
                id = m.reader.Read<EntityID>() + std::any_cast<EntityID>(m.userData);
            else
                m.writer.Write<EntityID>(id);
        }
    };
}

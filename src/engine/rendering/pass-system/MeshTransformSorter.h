#pragma once
#include <queue>

#include "ecs/ComponentConcepts.h"

namespace rendering
{
    struct MeshTransformSorter
    {
        struct Data
        {
            ecs::EntityID mesh;
            ecs::EntityID transform;

            auto operator<=>(const Data& other) const = default;
        };

        std::vector<Data> addQueue;
        std::vector<Data> removeQueue;
        std::vector<Data> sorted;

        void Refresh();
    };
}

#pragma once
#include "components/Model.h"
#include "components/Transform.h"
#include "ecs/EngineView.h"

namespace systems
{
    class ModelSystem
    {
        ecs::Engine& engine;
        ecs::EngineView<Model, Transform> view;

    public:
        explicit ModelSystem(ecs::Engine& engine) : engine(engine), view(engine) {}

        void Update(const Camera& camera);
    };
}

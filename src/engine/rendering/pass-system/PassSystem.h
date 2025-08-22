#pragma once
#include <variant>

#include "DefaultPass.h"
#include "IdentifyPass.h"
#include "StencilOutlinePass.h"
#include "SubMesh.h"
#include "ecs/Engine.h"

class RenderingEngine;

namespace rendering
{
    template <typename T>
    concept ManagedPass = ecs::one_of_v<T, DefaultPass, IdentifyPass, StencilOutlinePass>;

    class PassSystem
    {
        RenderingResources& resources;

        DefaultPass defaultPass;
        IdentifyPass identifyPass;
        StencilOutlinePass stencilOutlinePass;

    public:

        PassSystem(RenderingResources& resources, RenderingEngine& renderer, ecs::Engine& engine)
            : resources(resources), defaultPass(renderer, engine), identifyPass(renderer, engine), stencilOutlinePass(renderer, engine) {}

        template <ManagedPass T>
        T& GetPass()
        {
            if constexpr (std::is_same_v<DefaultPass, T>)
                return defaultPass;
            if constexpr (std::is_same_v<IdentifyPass, T>)
                return identifyPass;
            if constexpr (std::is_same_v<StencilOutlinePass, T>)
                return stencilOutlinePass;
            throw std::exception();
        }

        void Init();

        static void Serialize(serial::Stream&) {}

        void Draw(VkCommandBuffer cmd);

        void Destroy();
    };
}

#pragma once

#include "MeshTransformSorter.h"
#include "PassComponent.h"
#include "components/Transform.h"
#include "ecs/EngineView.h"
#include "glm/mat4x4.hpp"
#include "rendering/utility/Descriptors.h"


class StencilOutlinePass
{
public:
    struct PushConstants
    {
        glm::mat4 matrix;
        VkDeviceAddress vertexBuffer;
    };

    RenderingEngine& renderer;
    ecs::Engine& engine;
    VkDevice device = nullptr;

    VkPipelineLayout maskLayout = nullptr;
    VkPipeline maskPipeline = nullptr;
    VkPipelineLayout outlineLayout = nullptr;
    VkPipeline outlinePipeline = nullptr;

    ecs::EngineView<Transform, PassComponent<StencilOutlinePass>> view;
    rendering::MeshTransformSorter sorter;

    StencilOutlinePass(RenderingEngine& renderer, ecs::Engine& engine)
        : renderer(renderer),
          engine(engine), view(engine) {}

    void Init();

    void Draw(VkCommandBuffer cmd);

    void Destroy();
};

#pragma once

#include "PassComponent.h"
#include "SubMesh.h"
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
    DescriptorWriter properties {};
    ecs::EngineView<SubMesh, PassComponent<StencilOutlinePass>> view;

    StencilOutlinePass(RenderingEngine& renderer, ecs::Engine& engine)
        : renderer(renderer),
          engine(engine), view(engine) {}

    void Init();

    void Draw(VkCommandBuffer cmd);

    void Destroy();
};

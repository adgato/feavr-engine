#pragma once

#include "MeshTransformSorter.h"
#include "PassComponent.h"
#include "components/Model.h"
#include "ecs/EngineView.h"
#include "glm/mat4x4.hpp"
#include "rendering/CommonSetLayouts.h"
#include "rendering/resources/RenderingResources.h"
#include "rendering/utility/Descriptors.h"

namespace serial
{
    class Stream;
}

class IdentifyPass
{
    rendering::Buffer<rendering::GlobalSceneData> pixelSceneBuffer[rendering::FRAME_OVERLAP];

public:
    struct PushConstants
    {
        glm::mat4 matrix;
        VkDeviceAddress vertexBuffer;
        uint32_t identifier;
    };

    RenderingEngine& renderer;
    ecs::Engine& engine;
    VkDevice device = nullptr;

    ecs::EngineView<Model, PassComponent<IdentifyPass>> view;
    rendering::MeshTransformSorter sorter;

    DescriptorWriter pixelSceneProperties {};

    VkPipelineLayout layout = nullptr;
    VkPipeline pipeline = nullptr;

    IdentifyPass(RenderingEngine& renderer, ecs::Engine& engine)
        : renderer(renderer),
          engine(engine), view(engine) {}

    void Init();

    void Draw(VkCommandBuffer cmd, size_t frame, const rendering::GlobalSceneData& pixelSceneData);

    void Destroy();
};

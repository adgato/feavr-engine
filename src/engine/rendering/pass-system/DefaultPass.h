#pragma once

#include "MeshTransformSorter.h"
#include "PassComponent.h"
#include "components/Model.h"
#include "ecs/EngineView.h"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "rendering/utility/Descriptors.h"
#include "rendering/utility/DescriptorSetLayoutInfo.h"

namespace serial
{
    class Stream;
}

class DefaultPass
{
public:
    struct MaterialConstants
    {
        glm::vec4 colorFactors;
        glm::vec4 metal_rough_factors;
        //padding, we need it anyway for uniform buffers
        glm::vec4 extra[14];
    };

    struct PushConstants
    {
        glm::mat4 matrix;
        VkDeviceAddress vertexBuffer;
    };

    RenderingEngine& renderer;
    ecs::Engine& engine;
    VkDevice device = nullptr;

    ecs::EngineView<Model, PassComponent<DefaultPass>> view;
    rendering::MeshTransformSorter sorter;

    VkPipelineLayout layout = nullptr;
    VkPipeline pipeline = nullptr;
    DescriptorWriter properties {};
    rendering::Buffer<MaterialConstants> matConstProperty {};

    DescriptorSetLayoutInfo materialLayout {};

    DefaultPass(RenderingEngine& renderer, ecs::Engine& engine)
        : renderer(renderer),
          engine(engine), view(engine) {}

    void Init();

    void Draw(VkCommandBuffer cmd);

    void Destroy();
};

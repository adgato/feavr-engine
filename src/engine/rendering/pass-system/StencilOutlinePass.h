#pragma once
#include <span>

#include "PassComponent.h"
#include "SubMesh.h"
#include "ecs/EngineView.h"
#include "glm/mat4x4.hpp"
#include "rendering/utility/Descriptors.h"
#include "rendering/utility/DescriptorSetLayoutInfo.h"

namespace rendering
{
    struct Mesh;
}

namespace rendering::passes
{
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

        void Draw(VkCommandBuffer cmd, const std::span<Mesh>& meshes);

        void Destroy();
    };
}
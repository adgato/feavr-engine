#pragma once
#include <span>

#include "PassComponent.h"
#include "SubMesh.h"
#include "ecs/EngineView.h"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "rendering/utility/Descriptors.h"
#include "rendering/utility/DescriptorSetLayoutInfo.h"

namespace serial {
    class Stream;
}

namespace rendering
{
    struct Mesh;
}

namespace rendering::passes
{
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

        VulkanEngine* engine = nullptr;
        VkDevice device = nullptr;

        ecs::EngineView<SubMesh, PassComponent<DefaultPass>> view;

        VkPipelineLayout layout = nullptr;
        VkPipeline pipeline = nullptr;
        DescriptorWriter properties {};
        Buffer<MaterialConstants> matConstProperty;

        DescriptorSetLayoutInfo materialLayout;

        void Init(VulkanEngine* engine);

        void Draw(VkCommandBuffer cmd, const std::span<Mesh>& meshes);

        void Destroy();
    };
}

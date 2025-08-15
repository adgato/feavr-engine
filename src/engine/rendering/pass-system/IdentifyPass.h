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
    class IdentifyPass
    {
    public:
        struct PushConstants
        {
            glm::mat4 matrix;
            VkDeviceAddress vertexBuffer;
            uint32_t identifier;
        };

        VulkanEngine* engine = nullptr;
        VkDevice device = nullptr;
        ecs::EngineView<SubMesh, PassComponent<IdentifyPass>> view;

        VkPipelineLayout layout = nullptr;
        VkPipeline pipeline = nullptr;

        void Init(VulkanEngine* engine);

        void IdentifySubMeshesOf(ecs::Entity transform, std::vector<ecs::EntityID>& outSubMeshes);

        void Draw(VkCommandBuffer cmd, const std::span<Mesh>& meshes);

        void Destroy();
    };
}

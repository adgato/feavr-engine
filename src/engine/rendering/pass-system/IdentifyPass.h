#pragma once
#include <span>

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

        VkPipelineLayout layout = nullptr;
        VkPipeline pipeline = nullptr;

        void Init(VulkanEngine* engine);

        void Draw(VkCommandBuffer cmd, const std::span<Mesh>& meshes);

        void Destroy();
    };
}

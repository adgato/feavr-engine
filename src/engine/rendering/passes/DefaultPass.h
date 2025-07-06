#pragma once
#include "../SubMesh.h"
#include "Pass.h"
#include "PassInstance.h"
#include "vk_descriptors.h"

namespace rendering::passes
{
    class DefaultPass final : public Pass
    {
    public:
        DescriptorSetLayoutInfo materialLayout;
    protected:
        VkPipelineLayout CreateLayout() override;
        VkPipeline CreatePipeline(VkPipelineLayout pipelineLayout) override;

    public:
        
        struct PushConstants
        {
            glm::mat4 matrix;
            VkDeviceAddress vertexBuffer;
        };

        void ConfigurePass(VkCommandBuffer cmd) override;
        void DrawMesh(VkCommandBuffer cmd, const SubMesh& submesh) override;
        void Destroy() override;
    };

    template<> class PassInst<DefaultPass> final : public PassInstance
    {
    public:
        DescriptorWriter properties;
        
        struct MaterialConstants
        {
            glm::vec4 colorFactors;
            glm::vec4 metal_rough_factors;
            //padding, we need it anyway for uniform buffers
            glm::vec4 extra[14];
        };
        
        Buffer<MaterialConstants> matConstProperty;

        void Init(const std::shared_ptr<DefaultPass>& defaultPass);
        void Destroy() override;

        void ConfigureInstance(VkCommandBuffer cmd) override;
    };


}

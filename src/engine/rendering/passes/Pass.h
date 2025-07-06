#pragma once
#include "ecs/Engine.h"
#include "rendering/SubMesh.h"

class VulkanEngine;

namespace rendering::passes
{
    class Pass
    {
    public:
        VulkanEngine* engine = nullptr;
        ecs::EntityManager* manager = nullptr;
        VkDevice device = nullptr;
        
        VkPipelineLayout layout = nullptr;
        VkPipeline pipeline = nullptr;
        
        void Init(VulkanEngine* e, ecs::EntityManager* m);
        virtual void Destroy();
        
        Pass() = default;
        virtual ~Pass() = default;
        Pass(const Pass& other) = default;
        Pass& operator =(const Pass& other) = default;
        Pass(Pass&& other) = default;
        Pass& operator =(Pass&& other) = default;

        virtual void ConfigurePass(VkCommandBuffer cmd) = 0;
        virtual void DrawMesh(VkCommandBuffer cmd, const SubMesh& submesh) = 0;

    protected:
        virtual VkPipelineLayout CreateLayout() = 0;
        virtual VkPipeline CreatePipeline(VkPipelineLayout layout) = 0;
    };
}

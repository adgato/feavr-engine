#include "Pass.h"

#include "vk_engine.h"

namespace rendering::passes
{
    void Pass::Init(VulkanEngine* e, ecs::EntityManager* m)
    {
        engine = e;
        manager = m;
        device = e->device;
        layout = CreateLayout();
        
        pipeline = CreatePipeline(layout);
    }

    void Pass::Destroy()
    {
        vkDestroyPipelineLayout(device, layout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);

        layout = nullptr;
        pipeline = nullptr;
    }
}
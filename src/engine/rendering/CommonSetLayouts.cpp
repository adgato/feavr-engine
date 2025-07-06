#include "CommonSetLayouts.h"
#include "shader_descriptors.h"

using namespace shader_layouts::global;

namespace rendering
{
    void CommonSetLayouts::Init(const VkDevice _device)
    {
        device = _device;
        GPUSceneData = shader_layouts::CreateSetLayout(_device, &sceneData_bindings);
    }

    void CommonSetLayouts::Destroy()
    {
        vkDestroyDescriptorSetLayout(device, GPUSceneData.set, nullptr);
        GPUSceneData.set = nullptr;
    }
}


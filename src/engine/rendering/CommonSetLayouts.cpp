#include "CommonSetLayouts.h"

using namespace shader_layouts::global;

namespace rendering
{
    void CommonSetLayouts::Init(const VkDevice _device)
    {
        device = _device;
        sceneDataLayout = shader_layouts::CreateSetLayout(_device, &sceneData_bindings);
    }

    void CommonSetLayouts::Destroy()
    {
        vkDestroyDescriptorSetLayout(device, sceneDataLayout.set, nullptr);
        sceneDataLayout.set = nullptr;
    }
}
#pragma once
#include "pass-system/shader_descriptors.h"

namespace rendering
{
    class CommonSetLayouts
    {
        VkDevice device = nullptr;

    public:
        DescriptorSetLayoutInfo GPUSceneData;
        std::vector<VkDescriptorSetLayoutBinding> sceneData_bindings = {shader_layouts::global::SceneData_binding};

        void Init(VkDevice _device);

        void Destroy();
    };
}

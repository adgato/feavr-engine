#pragma once
#include "shader_descriptors.h"
#include "vk_types.h"

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

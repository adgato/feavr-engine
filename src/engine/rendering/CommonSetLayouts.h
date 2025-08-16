#pragma once
#include "glm/mat4x4.hpp"
#include "pass-system/shader_descriptors.h"

namespace rendering
{
    struct GlobalSceneData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
    };

    class CommonSetLayouts
    {
        VkDevice device = nullptr;

    public:
        DescriptorSetLayoutInfo sceneDataLayout;
        std::vector<VkDescriptorSetLayoutBinding> sceneData_bindings = { shader_layouts::global::SceneData_binding };

        void Init(VkDevice _device);

        void Destroy();
    };
}

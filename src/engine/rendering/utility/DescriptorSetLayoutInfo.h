#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

struct DescriptorSetLayoutInfo
{
    VkDescriptorSetLayout set;
    std::vector<VkDescriptorSetLayoutBinding>* bindings;
};

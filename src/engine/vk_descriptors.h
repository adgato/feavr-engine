#pragma once

#include <vector>
#include <vk_types.h>
#include <deque>
#include <map>
#include <span>

#include "rendering/Buffer.h"
#include "rendering/Image.h"

class DescriptorAllocator
{
public:
    VkDevice device = nullptr;

private:
    std::vector<VkDescriptorPool> pools;

    std::array<VkDescriptorPoolSize, 11> lastPoolSizes = {
        {
            // Most common - textures and uniforms
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 300},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 300},

            // Common for modern rendering
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 200},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 50},

            // Less common but still used
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 50},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 50},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 20},

            // Specialized use cases
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 5}
        }
    };
    std::array<VkDescriptorPoolSize, 11> poolSizes = {
        {
            // Most common - textures and uniforms
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0},

            // Common for modern rendering
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0},

            // Less common but still used
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 0},

            // Specialized use cases
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0}
        }
    };
    uint32_t lastSetCount = 1000;
    uint32_t setCount = 0;

    void AllocPool();

public:
    void Init(VkDevice _device);

    void ResetPools();

    void DestroyPools();

    VkDescriptorSet Alloc(DescriptorSetLayoutInfo layout);
};

class DescriptorWriter
{
    VkDevice device = nullptr;
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

public:
    VkDescriptorSet descriptorSet;

    void AllocateSet(DescriptorAllocator& allocator, DescriptorSetLayoutInfo layout);

    void StageCombinedImage(const VkDescriptorSetLayoutBinding& binding, VkImageView image, VkSampler sampler, VkImageLayout layout);

    void StageBuffer(const VkDescriptorSetLayoutBinding& binding, VkBuffer buffer, size_t size, size_t offset = 0);

    void StageSampler(const VkDescriptorSetLayoutBinding& binding, const VkSampler sampler)
    {
        assert(binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER);
        StageCombinedImage(binding, VK_NULL_HANDLE, sampler, VK_IMAGE_LAYOUT_UNDEFINED);
    }

    void StageImage(const VkDescriptorSetLayoutBinding& binding, const rendering::Image& image)
    {
        assert(binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        StageCombinedImage(binding, image.imageView, VK_NULL_HANDLE, image.currentLayout);
    }

    template<typename T>
    void StageBuffer(const VkDescriptorSetLayoutBinding& binding, const rendering::Buffer<T>& buffer, const size_t offset = 0)
    {
        StageBuffer(binding, buffer.buffer, sizeof(T) * buffer.count, offset);
    }

    void PerformWrites();

    void DiscardWrites();
};

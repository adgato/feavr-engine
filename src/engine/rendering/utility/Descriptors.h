#pragma once

#include <vector>
#include <forward_list>

#include "rendering/resources/Buffer.h"
#include "rendering/resources/Image.h"

struct DescriptorSetLayoutInfo;

namespace rendering
{
    struct ResourceHandles;
}


class DescriptorAllocator
{
    VkDevice device = nullptr;

    std::vector<VkDescriptorPool> pools;

    static std::array<VkDescriptorPoolSize, 11> initialPoolSizes;

    std::array<VkDescriptorPoolSize, 11> poolSizes = initialPoolSizes;
    uint32_t setCount = 1000;

    void AllocPool();

public:
    void Init(VkDevice _device);

    void ResetPools();

    void DestroyPools();

    VkDescriptorSet Alloc(DescriptorSetLayoutInfo layout);
};


// conventional set index meaning
enum DescriptorSetIndex
{
    PerFrameGlobalSetIdx = 0,
    PerPassSetIdx = 1,
    // convenient for material instances
    PerGroupSetIdx = 2,
    // convenient for geometry instancing
    PerMeshSetIdx = 3,
    PerEntitySetIdx = 4
};

class DescriptorWriter
{
    VkDevice device = nullptr;
    std::forward_list<VkDescriptorImageInfo> imageInfos;
    std::forward_list<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

public:
    VkDescriptorSet descriptorSet;

    void AllocateSet(const rendering::ResourceHandles& resource, DescriptorSetLayoutInfo layout);

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

    template <typename T>
    void StageBuffer(const VkDescriptorSetLayoutBinding& binding, const rendering::Buffer<T>& buffer, const size_t offset = 0)
    {
        StageBuffer(binding, buffer.buffer, sizeof(T) * buffer.count, offset);
    }

    void PerformWrites();

    void DiscardWrites();
};

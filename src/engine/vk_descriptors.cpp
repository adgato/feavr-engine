#include <vk_descriptors.h>
#include <cassert>
#include "vk_new.h"

void DescriptorAllocator::AllocPool()
{
    for (uint32_t i = 0; i < poolSizes.size(); ++i)
    {
        lastPoolSizes[i].descriptorCount += poolSizes[i].descriptorCount;
        poolSizes[i].descriptorCount = 0;
    }
    lastSetCount += setCount;
    setCount = 0;

    auto pool_info = vkinit::New<VkDescriptorPoolCreateInfo>(); {
        pool_info.flags = 0;
        pool_info.maxSets = lastSetCount;
        pool_info.poolSizeCount = static_cast<uint32_t>(lastPoolSizes.size());
        pool_info.pPoolSizes = lastPoolSizes.data();
    }
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &pools.emplace_back()));
}

void DescriptorAllocator::Init(const VkDevice _device)
{
    device = _device;
    AllocPool();
}

void DescriptorAllocator::ResetPools()
{
    const VkDescriptorPool pool = pools.back();
    VK_CHECK(vkResetDescriptorPool(device, pool, 0));

    if (pools.size() > 1)
    {
        pools.pop_back();
        DestroyPools();
        pools.push_back(pool);
    }

    for (uint32_t i = 0; i < poolSizes.size(); ++i)
        poolSizes[i].descriptorCount = 0;
    setCount = 0;
}

void DescriptorAllocator::DestroyPools()
{
    for (const VkDescriptorPool fullPool : pools)
        vkDestroyDescriptorPool(device, fullPool, nullptr);
    pools.clear();
}

VkDescriptorSet DescriptorAllocator::Alloc(const DescriptorSetLayoutInfo layout)
{
    // track what types are allocated
    for (const VkDescriptorSetLayoutBinding& binding : *layout.bindings)
        for (auto& [type, descriptorCount] : poolSizes)
            if (binding.descriptorType == type)
            {
                descriptorCount += binding.descriptorCount;
                break;
            }
    setCount++;

    VkDescriptorSet ds;
    auto allocInfo = vkinit::New<VkDescriptorSetAllocateInfo>(); {
        allocInfo.descriptorPool = pools.back();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout.set;
    }

    if (const VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);
        result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        // use new pool instead
        AllocPool();
        allocInfo.descriptorPool = pools.back();
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }
    return ds;
}



void DescriptorWriter::AllocateSet(DescriptorAllocator& allocator, const DescriptorSetLayoutInfo layout)
{
    device = allocator.device;
    descriptorSet = allocator.Alloc(layout);
    DiscardWrites();
}

void DescriptorWriter::StageCombinedImage(const VkDescriptorSetLayoutBinding& binding, const VkImageView image, const VkSampler sampler, const VkImageLayout layout)
{
    const VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo {
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
    });

    auto write = vkinit::New<VkWriteDescriptorSet>(); {
        write.dstBinding = binding.binding;
        write.dstSet = descriptorSet;
        write.descriptorCount = binding.descriptorCount;
        write.descriptorType = binding.descriptorType;
        write.pImageInfo = &info;
    }
    writes.push_back(write);
}

void DescriptorWriter::StageBuffer(const VkDescriptorSetLayoutBinding& binding, const VkBuffer buffer, const size_t size, const size_t offset /*= 0*/)
{
    const VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo {
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    auto write = vkinit::New<VkWriteDescriptorSet>(); {
        write.dstBinding = binding.binding;
        write.dstSet = descriptorSet;
        write.descriptorCount = binding.descriptorCount;
        write.descriptorType = binding.descriptorType;
        write.pBufferInfo = &info;
    }
    writes.push_back(write);
}

void DescriptorWriter::PerformWrites()
{
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    DiscardWrites();
}

void DescriptorWriter::DiscardWrites()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

#include "Descriptors.h"

#include "DescriptorSetLayoutInfo.h"
#include "VulkanCheck.h"
#include "VulkanNew.h"
#include "rendering/resources/RenderingResources.h"

std::array<VkDescriptorPoolSize, 11> DescriptorAllocator::initialPoolSizes = {
    {
        // Most common - textures and uniforms
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 300 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 300 },

        // Common for modern rendering
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 200 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 50 },

        // Less common but still used
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 50 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 50 },
        { VK_DESCRIPTOR_TYPE_SAMPLER, 20 },

        // Specialized use cases
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 5 }
    }
};

// the next pool sizes equal the sum of allocations since the last reset (plus an offset)
// this means the next pool by itself is large enough to hold all allocations since that reset
void DescriptorAllocator::AllocPool()
{
    auto pool_info = vkinit::New<VkDescriptorPoolCreateInfo>(); {
        pool_info.flags = 0;
        pool_info.maxSets = setCount;
        pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        pool_info.pPoolSizes = poolSizes.data();
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
    if (pools.size() == 0)
        return;

    if (pools.size() > 1)
    {
        DestroyPools();
        AllocPool();
    }
    else
        VK_CHECK(vkResetDescriptorPool(device, pools.back(), 0));

    for (uint32_t i = 0; i < initialPoolSizes.size(); ++i)
        poolSizes[i].descriptorCount = initialPoolSizes[i].descriptorCount;

    setCount = 1000;
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



void DescriptorWriter::AllocateSet(const VkDevice device, DescriptorAllocator& allocator, DescriptorSetLayoutInfo layout)
{
    this->device = device;
    descriptorSet = allocator.Alloc(layout);
    DiscardWrites();
}

void DescriptorWriter::StageCombinedImage(const VkDescriptorSetLayoutBinding& binding, const VkImageView image, const VkSampler sampler, const VkImageLayout layout)
{
    const VkDescriptorImageInfo& info = imageInfos.emplace_front(VkDescriptorImageInfo {
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
    const VkDescriptorBufferInfo& info = bufferInfos.emplace_front(VkDescriptorBufferInfo {
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

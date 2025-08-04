#pragma once

#include <cstring>
#include <functional>

#include "vk_mem_alloc.h"
#include "rendering/utility/VulkanCheck.h"

namespace rendering
{
    enum class HostAccess
    {
        NONE = 0,
        RANDOM = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        SEQUENTIAL_WRITE = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    };

    template <typename T>
    class Buffer
    {
        VmaAllocator allocator = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        T* mappedData = nullptr;

    public:
        VkBuffer buffer = VK_NULL_HANDLE;
        uint32_t count = 0;

        static Buffer Allocate(const VmaAllocator allocator, const size_t count, const VkBufferUsageFlags usage, const HostAccess hostAccess,
                               const VmaMemoryUsage memoryType = VMA_MEMORY_USAGE_AUTO)
        {
            // allocate buffer
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.pNext = nullptr;
            bufferInfo.size = sizeof(T) * count;

            bufferInfo.usage = usage;

            VmaAllocationCreateInfo vmaAllocInfo = {};
            vmaAllocInfo.usage = memoryType;

            if (hostAccess != HostAccess::NONE)
                vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | static_cast<VmaAllocationCreateFlagBits>(hostAccess);

            Buffer newBuffer;

            VmaAllocationInfo info;

            newBuffer.count = static_cast<uint32_t>(count);
            newBuffer.allocator = allocator;
            VK_CHECK(vmaCreateBuffer(newBuffer.allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, &info));

            newBuffer.mappedData = static_cast<T*>(info.pMappedData);

            return newBuffer;
        }

        T* Access() const
        {
            assert(buffer != VK_NULL_HANDLE);
            return mappedData;
        }

        void Write(const T* data) const
        {
            assert(buffer != VK_NULL_HANDLE);
            std::memcpy(mappedData, data, sizeof(T) * count);
        }

        void Destroy()
        {
            if (buffer == VK_NULL_HANDLE)
                return;

            vmaDestroyBuffer(allocator, buffer, allocation);
            buffer = VK_NULL_HANDLE;
        }
    };
}

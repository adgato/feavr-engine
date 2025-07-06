#pragma once

#include "vk_types.h"

namespace rendering
{
    template <typename T>
    class Buffer
    {
    public:
        VmaAllocator allocator = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        uint32_t count = 0;
    
        static Buffer Allocate(const VmaAllocator allocator, const size_t count, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage)
        {
            // allocate buffer
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.pNext = nullptr;
            bufferInfo.size = sizeof(T) * count;

            bufferInfo.usage = usage;

            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = memoryUsage;
            vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            Buffer newBuffer;

            newBuffer.count = static_cast<uint32_t>(count);
            newBuffer.allocator = allocator;
            VK_CHECK(vmaCreateBuffer(newBuffer.allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr));

            return newBuffer;
        }

        void Access(const std::function<void(T*)>& action) const
        {
            void* mappedData;
            vmaMapMemory(allocator, allocation, &mappedData);
            action(static_cast<T*>(mappedData));
            vmaUnmapMemory(allocator, allocation);
        }
        
        void Write(const T* data) const
        {
            void* mappedData;
            vmaMapMemory(allocator, allocation, &mappedData);
            std::memcpy(mappedData, data, sizeof(T) * count);
            vmaUnmapMemory(allocator, allocation);
        }
        
        void Destroy()
        {
            if (buffer == VK_NULL_HANDLE)
                return;
            
            vmaDestroyBuffer(allocator, buffer, allocation);
            buffer = VK_NULL_HANDLE;
        }
    };

    class RawBuffer
    {
        VmaAllocator allocator = nullptr;
    public:
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    
        static RawBuffer Allocate(const VmaAllocator allocator, const size_t size, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage)
        {
            // allocate buffer
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.pNext = nullptr;
            bufferInfo.size = size;

            bufferInfo.usage = usage;

            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = memoryUsage;
            vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            RawBuffer newBuffer;

            newBuffer.allocator = allocator;
            VK_CHECK(vmaCreateBuffer(newBuffer.allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

            return newBuffer;
        }

        void Access(const std::function<void(void*)>& action) const
        {
            void* mappedData;
            vmaMapMemory(allocator, allocation, &mappedData);
            action(mappedData);
            vmaUnmapMemory(allocator, allocation);
        }

        void Write(const void* data) const
        {
            void* mappedData;
            vmaMapMemory(allocator, allocation, &mappedData);
            std::memcpy(mappedData, data, info.size);
            vmaUnmapMemory(allocator, allocation);
        }
        
        void Destroy() const
        {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    };
}
// ReSharper disable CppMemberFunctionMayBeConst
#pragma once
#include <type_traits>
#include <cstring>

namespace ecs
{
    struct RawArray
    {
        std::byte* data = nullptr;
        size_t stride = 0;
        
        RawArray() = default;

        explicit RawArray(const TypeInfo typeInfo)
        {
            stride = (typeInfo.size + typeInfo.align - 1) & ~(typeInfo.align - 1);
        }

        ~RawArray()
        {
            if (data)
                free(data);
            data = nullptr;
        }
        RawArray(const RawArray&) = delete;
        RawArray& operator=(const RawArray&) = delete;
        RawArray(RawArray&& other) noexcept
        {
            data = other.data;
            stride = other.stride;
    
            other.data = nullptr;
        }
        RawArray& operator=(RawArray&& other) noexcept
        {
            data = other.data;
            stride = other.stride;
    
            other.data = nullptr;
            return *this;
        }

        std::byte* GetElem(const size_t index) const
        {
            return data + index * stride;
        }
        
        void SetElem(const size_t index, const std::byte* src)
        {
            std::memcpy(data + index * stride, src, stride);
        }

        void ReplaceElem(const size_t dst, const size_t src)
        {
            std::memcpy(data + dst * stride, data + src * stride, stride);
        }
        
        bool Realloc(const size_t capacity)
        {
            void* new_data = realloc(data, capacity * stride);
            if (!new_data)
                return false;
            data = static_cast<std::byte*>(new_data);
            return true;
        }
    };
}

#pragma once
#include <cstring>

#include "Common.h"

namespace ecs
{
    struct RawArray
    {
        std::byte* data = nullptr;
        size_t stride = 0;
        size_t align = 0;

        RawArray() = default;

        explicit RawArray(const TypeInfo& typeInfo) noexcept
        {
            stride = (typeInfo.size + typeInfo.align - 1) & ~(typeInfo.align - 1);
            align = typeInfo.align;
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

        void SetElem(const size_t index, const std::byte* src) const
        {
            std::memcpy(data + index * stride, src, stride);
        }

        void ReplaceElem(const size_t dst, const size_t src) const
        {
            std::memcpy(data + dst * stride, data + src * stride, stride);
        }

        bool Realloc(const size_t new_capacity, const size_t old_capacity);
    };
}

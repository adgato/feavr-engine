#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>

#include "ComponentConcepts.h"

namespace ecs
{
    struct RawArray
    {
        std::byte* data = nullptr;
        size_t stride = 0;

        RawArray() = default;

        explicit RawArray(const TypeInfo& typeInfo) noexcept
        {
            assert(typeInfo.align <= alignof(std::max_align_t));
            stride = (typeInfo.size + typeInfo.align - 1) & ~(typeInfo.align - 1);
        }

        ~RawArray()
        {
            if (data)
                std::free(data);
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
            assert(data);
            return data + index * stride;
        }

        void SetElem(const size_t index, const std::byte* src) const
        {
            assert(data);
            std::memcpy(data + index * stride, src, stride);
        }

        void ReplaceElem(const size_t dst, const size_t src) const
        {
            assert(data);
            std::memcpy(data + dst * stride, data + src * stride, stride);
        }

        bool Realloc(const size_t capacity)
        {
            return data = static_cast<std::byte*>(std::realloc(data, capacity * stride));
        }
    };
}

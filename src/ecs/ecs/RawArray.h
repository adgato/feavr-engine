#pragma once
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "ComponentConcepts.h"


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

        bool Realloc(size_t new_capacity, size_t old_capacity);
    };
}

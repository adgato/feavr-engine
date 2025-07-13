// ReSharper disable CppMemberFunctionMayBeConst
#pragma once
#include <cstring>

namespace ecs
{
    struct RawArray
    {
        std::byte* data = nullptr;
        size_t stride = 0;
        size_t align = 0;

        RawArray() = default;

        explicit RawArray(const TypeInfo typeInfo) noexcept
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

        void SetElem(const size_t index, const std::byte* src)
        {
            std::memcpy(data + index * stride, src, stride);
        }

        void ReplaceElem(const size_t dst, const size_t src)
        {
            std::memcpy(data + dst * stride, data + src * stride, stride);
        }

        bool Realloc(const size_t new_capacity, const size_t old_capacity)
        {
            if (align <= alignof(std::max_align_t))
            {
                // realloc is safe for standard alignments
                void* new_data = realloc(data, new_capacity * stride);
                if (!new_data)
                    return false;
                data = static_cast<std::byte*>(new_data);
                return true;
            }

            // Manual reallocation for over-aligned types
            void* new_data = std::aligned_alloc(align, new_capacity * stride);
            if (!new_data)
                return false;

            if (data)
            {
                // Copy existing data
                const size_t copy_size = std::min(old_capacity, new_capacity) * stride;
                std::memcpy(new_data, data, copy_size);
                std::free(data);
            }

            data = static_cast<std::byte*>(new_data);
            return true;
        }
    };
}

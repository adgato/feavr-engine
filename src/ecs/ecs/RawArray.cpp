#include "RawArray.h"

#include <algorithm>

namespace ecs
{
    bool RawArray::Realloc(const size_t new_capacity, const size_t old_capacity)
    {
        if (align <= alignof(std::max_align_t))
        {
            // realloc is safe for standard alignments
            void* new_data = std::realloc(data, new_capacity * stride);
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
}

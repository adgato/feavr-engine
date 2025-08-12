#pragma once

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

namespace rendering {
    class Image;
}

namespace rendering::utility
{
    void Screenshot(VmaAllocator vmaAllocator, const char* fileToSave, Image& renderTarget);
}

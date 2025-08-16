#pragma once

namespace rendering {
    class RenderingResources;
}

struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;

namespace rendering {
    class Image;
}

namespace rendering::utility
{
    void Screenshot(const RenderingResources& resources, VmaAllocator vmaAllocator, const char* fileToSave, Image& renderTarget);
}

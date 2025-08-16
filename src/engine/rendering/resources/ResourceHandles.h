#pragma once
#pragma once
#include <SDL_video.h>

#include "rendering/utility/Descriptors.h"

namespace rendering
{
    struct ResourceHandles
    {
        SDL_Window* window;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHR surface;
        VkDevice device;
        VmaAllocator vmaAllocator;

        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamily;

        std::unique_ptr<DescriptorAllocator> globalAllocator;

        operator VmaAllocator const() const { return vmaAllocator; }
        operator VkDevice const() const { return device; }

        void Destroy() const;
    };
}

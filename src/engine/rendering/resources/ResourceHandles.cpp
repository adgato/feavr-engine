#include "ResourceHandles.h"

#include "VkBootstrap.h"

namespace rendering
{
    void ResourceHandles::Destroy() const
    {
        vmaDestroyAllocator(vmaAllocator);
        globalAllocator->DestroyPools();

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyDevice(device, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debug_messenger);
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
    }

}

#include "CommonTextures.h"

#include <array>
#include <glm/packing.hpp>

#include "vk_engine.h"

std::array<uint32_t, 256> CreateErrorCheckerboard()
{
    const uint32_t black = packUnorm4x8(glm::vec4(0, 0, 0, 0));
    const uint32_t magenta = packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 256> pixels;
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++)
            pixels[y * 16 + x] = x % 2 != y % 2 ? magenta : black;
    return pixels;
}

void rendering::CommonTextures::Init(VulkanEngine* engine, SwapchainRenderer* swapchainRenderer)
{
    errorCheckerboard = Image::Allocate(engine->vmaAllocator, engine->device, VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    const std::array<uint32_t, 256> checkerboard = CreateErrorCheckerboard();
    errorCheckerboard.Write(swapchainRenderer, checkerboard.data());
}

void rendering::CommonTextures::Destroy()
{
    errorCheckerboard.Destroy();
}

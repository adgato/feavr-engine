#include "Screenshot.h"

#include "stb_image_write.h"
#include "glm/gtc/packing.hpp"
#include "rendering/resources/EngineResources.h"
#include "rendering/resources/Image.h"

namespace rendering::utility
{
    static uint32_t hash(uint32_t x)
    {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

    static float uintToFloat01(uint32_t x)
    {
        return (x & 0x00FFFFFF) / static_cast<float>(0x01000000);
    }

    static glm::vec4 randomColour(uint32_t seed)
    {
        return glm::vec4(
            uintToFloat01(hash(seed)),
            uintToFloat01(hash(seed + 1)),
            uintToFloat01(hash(seed + 2)),
            1
        );
    }

    void Screenshot(const VmaAllocator vmaAllocator, const char* fileToSave, Image& renderTarget)
    {
        int componentsSize;
        switch (renderTarget.imageFormat)
        {
            case VK_FORMAT_R8G8B8A8_UNORM:
                componentsSize = 4;
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                componentsSize = 4;
                break;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                componentsSize = 8;
                break;
            case VK_FORMAT_R32_UINT:
                componentsSize = 4;
                break;
            default:
                fmt::println(stderr, "Unknown image format encountered when writing to {}.", fileToSave);
                return;
        }

        const size_t pixelCount = static_cast<size_t>(renderTarget.imageExtent.depth) * renderTarget.imageExtent.width * renderTarget.imageExtent.height;

        Buffer<std::byte> buffer = Buffer<std::byte>::Allocate(vmaAllocator, pixelCount * componentsSize,
                                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT, HostAccess::RANDOM, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        renderTarget.ReadFromRenderTarget(buffer.buffer);

        std::vector<uint32_t> convertedData;

        void* formattedData;
        switch (renderTarget.imageFormat)
        {
            case VK_FORMAT_R8G8B8A8_UNORM:
                formattedData = buffer.Access();
                break;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            {
                const uint64_t* packedPixels = reinterpret_cast<const uint64_t*>(buffer.Access());

                convertedData.resize(pixelCount);
                for (size_t i = 0; i < pixelCount; ++i)
                    convertedData[i] = glm::packUnorm4x8(glm::unpackHalf4x16(packedPixels[i]));

                formattedData = convertedData.data();
                break;
            }
            case VK_FORMAT_R32_UINT:
            {
                const uint32_t* packedPixels = reinterpret_cast<const uint32_t*>(buffer.Access());

                convertedData.resize(pixelCount);
                for (size_t i = 0; i < pixelCount; ++i)
                    convertedData[i] = glm::packUnorm4x8(randomColour(packedPixels[i]));

                formattedData = convertedData.data();
                break;
            }
            default:
                formattedData = buffer.Access();
                break;
        }

        constexpr int numComponents = 4;
        if (stbi_write_png(fileToSave, renderTarget.imageExtent.width, renderTarget.imageExtent.height, numComponents, formattedData, renderTarget.imageExtent.width * numComponents))
            fmt::println("Saved image to {}.", fileToSave);
        else
            fmt::println(stderr, "Failed to write image to {}.", fileToSave);

        buffer.Destroy();
    }
}

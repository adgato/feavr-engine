#include "Screenshot.h"

#include "stb_image_write.h"
#include "glm/gtc/packing.hpp"
#include "rendering/resources/EngineResources.h"
#include "rendering/resources/Image.h"

namespace rendering::utility
{
    void Screenshot(const VmaAllocator vmaAllocator, const char* fileToSave, Image& image)
    {
        int numComponents;
        int componentsSize;
        switch (image.imageFormat)
        {
            case VK_FORMAT_R8G8B8A8_UNORM:
                numComponents = 4;
                componentsSize = 4;
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                numComponents = 4;
                componentsSize = 4;
                break;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                numComponents = 4;
                componentsSize = 8;
                break;
            default:
                fmt::println(stderr, "Unknown image format encountered when writing to {}.", fileToSave);
                return;
        }

        const size_t pixelCount = static_cast<size_t>(image.imageExtent.depth) * image.imageExtent.width * image.imageExtent.height;

        Buffer<std::byte> buffer = Buffer<std::byte>::Allocate(vmaAllocator, pixelCount * componentsSize,
                                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT, HostAccess::RANDOM, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        image.Read(buffer);

        std::vector<uint32_t> convertedData;

        void* formattedData;
        switch (image.imageFormat)
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
            default:
                formattedData = buffer.Access();
                break;
        }

        if (stbi_write_png(fileToSave, image.imageExtent.width, image.imageExtent.height, numComponents, formattedData, image.imageExtent.width * numComponents))
            fmt::println("Saved image to {}.", fileToSave);
        else
            fmt::println(stderr, "Failed to write image to {}.", fileToSave);

        buffer.Destroy();
    }
}

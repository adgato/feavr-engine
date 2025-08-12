#pragma once
#include <functional>
#include <span>

#include "rendering/pass-system/Mesh.h"
#include "rendering/resources/ResourceHandles.h"

namespace rendering
{
    class Image;

    struct FrameCommand
    {
        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;

        VkCommandPool commandPool;
        VkCommandBuffer cmd;
    };

    constexpr uint32_t FRAME_OVERLAP = 2;
    constexpr VkExtent2D windowSize { 1700, 900 };
    constexpr VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    class EngineResources
    {
    public:
        FrameCommand frames[FRAME_OVERLAP];

        std::vector<Image> swapchainImages;

        VkCommandBuffer immCommandBuffer;
        VkFence immFence;
        VkCommandPool immCommandPool;

        uint32_t frameCount;

        VkSwapchainKHR swapchain;
        uint32_t currentSwapchainImageIndex;

        ResourceHandles resource;

        void Init();

        void ImmediateSumbit(std::function<void(VkCommandBuffer cmd)>&& function) const;

        Mesh UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) const;

        std::tuple<VkCommandBuffer, Image&> BeginFrame();

        void EndFrame();

        void Destroy();
    };
}

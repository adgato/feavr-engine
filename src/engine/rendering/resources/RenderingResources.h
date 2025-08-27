#pragma once
#include <functional>

#include "ResourceHandles.h"

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
    constexpr VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    constexpr float deltaTime = 1.f / 60.f;

    class RenderingResources
    {
        bool resizeRequested = false;

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

    private:

        void ResizeSwapchain();
        void CreateSwapchain(const VkExtent2D& windowSize);
        void DestroySwapchain();

    public:

        void Init();

        void ImmediateSumbit(std::function<void(VkCommandBuffer cmd)>&& function) const;

        std::tuple<VkCommandBuffer, Image&> BeginFrame();

        void EndFrame();

        void Destroy();
    };
}

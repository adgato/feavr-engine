#pragma once
#include "vk_types.h"
#include <functional>
#include <SDL_video.h>

#include "Image.h"

namespace rendering
{
    struct FrameCommand
    {
        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;

        VkCommandPool commandPool;
        VkCommandBuffer cmd;
    };

	struct DeviceData
	{
		SDL_Window* window;
		VkInstance instance;
		VkDebugUtilsMessengerEXT debug_messenger;
		VkPhysicalDevice physicalDevice;
		VkSurfaceKHR surface;
		VkDevice device;

		VkQueue graphicsQueue;
		uint32_t graphicsQueueFamily;

		void Destroy() const;
	};

	constexpr int FRAME_OVERLAP = 2;
	constexpr VkExtent2D windowSize { 1700 , 900 };
	constexpr VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	
    class SwapchainRenderer
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

    	DeviceData deviceData;


        void Init();

        void ImmediateSumbit(std::function<void(VkCommandBuffer cmd)>&& function) const;
        
        std::tuple<VkCommandBuffer, Image&> BeginFrame();
        void EndFrame();

        void Destroy();
    };
}

// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <ranges>
#include "vk_mem_alloc.h"

#include "camera.h"
#include "vk_descriptors.h"
#include "rendering/CommonSetLayouts.h"
#include "rendering/CommonTextures.h"
#include "rendering/passes/PassDirectory.h"
#include "rendering/Image.h"
#include "rendering/Material.h"
#include "rendering/PassManager.h"
#include "rendering/Swapchain.h"

struct FrameData
{
	DescriptorAllocator frameDescriptors;
	rendering::Buffer<GPUSceneData> sceneDataBuffer;
};

class VulkanEngine {
public:

	bool _isInitialized{ false };

	ecs::EntityManager entityManager;
	
	FrameData frameData[rendering::FRAME_OVERLAP];

	VmaAllocator vmaAllocator;
	VkDevice device;
	
	Camera mainCamera;
	
	float renderScale = 1.f;
	DescriptorAllocator descriptorAllocator;

	rendering::CommonSetLayouts commonSets;
	rendering::CommonTextures commonTextures;

	DescriptorWriter gpuSceneDescriptorSet;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;

	rendering::Image drawImage;
	rendering::Image depthImage;
	
	rendering::PassManager passManager;

	VkDescriptorPool imguiPool;

	GPUSceneData sceneData;

	rendering::Material<default_pass::Pass> defaultMaterial;

	void init(rendering::SwapchainRenderer* swapchainRenderer);

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw(uint32_t frameCount, VkCommandBuffer cmd, rendering::Image& targetImage);

	rendering::Mesh UploadMesh(const rendering::SwapchainRenderer* swapchainRenderer, std::span<uint32_t> indices, std::span<Vertex> vertices) const;

private:
	void init_swapchain(const rendering::DeviceData& deviceData);
	
	void init_descriptors();
	
	void init_imgui(const rendering::DeviceData& deviceData);
};

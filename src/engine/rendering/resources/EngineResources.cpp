#include "EngineResources.h"

#include <SDL.h>
#include "rendering/pass-system/Mesh.h"
#include <SDL_vulkan.h>
#include "VkBootstrap.h"
#include "rendering/utility/VulkanNew.h"

namespace rendering
{
    void EngineResources::Init()
    {
        constexpr bool useValidationLayers = true;

        SDL_Init(SDL_INIT_VIDEO);

        resource.window = SDL_CreateWindow(
            "Vulkan Engine",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            windowSize.width,
            windowSize.height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );

        vkb::InstanceBuilder builder;

        //make the vulkan instance, with basic debug features
        auto inst_ret = builder.set_app_name("Example Vulkan Application")
                .request_validation_layers(useValidationLayers)
                .use_default_debug_messenger()
                .require_api_version(1, 3, 0)
                .build();

        vkb::Instance vkb_inst = inst_ret.value();

        //grab the instance
        resource.instance = vkb_inst.instance;
        resource.debug_messenger = vkb_inst.debug_messenger;

        SDL_Vulkan_CreateSurface(resource.window, resource.instance, &resource.surface);

        VkPhysicalDeviceVulkan13Features features13 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features features12 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing = true;

        VkPhysicalDeviceFeatures features {};
        features.shaderInt64 = true;

        //use vkbootstrap to select a gpu.
        //We want a gpu that can write to the SDL surface and supports vulkan 1.2
        vkb::PhysicalDeviceSelector selector { vkb_inst };
        vkb::PhysicalDevice vkbPhysicalDevice = selector
                .set_minimum_version(1, 3)
                .set_required_features_13(features13)
                .set_required_features_12(features12)
                .set_required_features(features)
                .set_surface(resource.surface)
                .select()
                .value();

        //create the final vulkan device

        vkb::DeviceBuilder deviceBuilder { vkbPhysicalDevice };

        vkb::Device vkbDevice = deviceBuilder.build().value();

        // Get the VkDevice handle used in the rest of a vulkan application
        resource.device = vkbDevice.device;
        resource.physicalDevice = vkbPhysicalDevice.physical_device;

        // use vkbootstrap to get a Graphics queue
        resource.graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        resource.graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        //initialize the memory allocator

        vkb::SwapchainBuilder swapchainBuilder { resource.physicalDevice, resource.device, resource.surface };

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                //.use_default_format_selection()
                .set_desired_format(VkSurfaceFormatKHR { .format = imageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
                //use vsync present mode
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                .set_desired_extent(windowSize.width, windowSize.height)
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                .build()
                .value();

        swapchain = vkbSwapchain.swapchain;
        swapchainImages.clear();

        const std::vector<VkImage>& images = vkbSwapchain.get_images().value();
        const std::vector<VkImageView>& imageViews = vkbSwapchain.get_image_views().value();

        for (uint32_t i = 0; i < vkbSwapchain.image_count; ++i)
        {
            Image& image = swapchainImages.emplace_back();
            image.imageExtent = VkExtent3D { vkbSwapchain.extent.width, vkbSwapchain.extent.height, 1 };
            image.image = images[i];
            image.imageView = imageViews[i];
            image.renderer = this;
            image.allocation = VK_NULL_HANDLE;
            image.imageFormat = imageFormat;
            image.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
            image.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        //create a command pool for commands submitted to the graphics queue.
        //we also want the pool to allow for resetting of individual command buffers
        //create syncronization structures
        //one fence to control when the gpu has finished rendering the frame,
        //and 2 semaphores to syncronize rendering with swapchain
        //we want the fence to start signalled so we can wait on it on the first frame
        auto commandPoolInfo = vkinit::New<VkCommandPoolCreateInfo>(); {
            commandPoolInfo.queueFamilyIndex = resource.graphicsQueueFamily;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        }
        auto cmdAllocInfo = vkinit::New<VkCommandBufferAllocateInfo>(); {
            cmdAllocInfo.commandBufferCount = 1;
            cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        }
        auto fenceCreateInfo = vkinit::New<VkFenceCreateInfo>(); {
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        const auto semaphoreCreateInfo = vkinit::New<VkSemaphoreCreateInfo>();

        VK_CHECK(vkCreateCommandPool(resource.device, &commandPoolInfo, nullptr, &immCommandPool));
        cmdAllocInfo.commandPool = immCommandPool;
        VK_CHECK(vkAllocateCommandBuffers(resource.device, &cmdAllocInfo, &immCommandBuffer));
        VK_CHECK(vkCreateFence(resource.device, &fenceCreateInfo, nullptr, &immFence));

        for (auto& frame : frames)
        {
            VK_CHECK(vkCreateCommandPool(resource.device, &commandPoolInfo, nullptr, &frame.commandPool));
            cmdAllocInfo.commandPool = frame.commandPool;
            VK_CHECK(vkAllocateCommandBuffers(resource.device, &cmdAllocInfo, &frame.cmd));
            VK_CHECK(vkCreateFence(resource.device, &fenceCreateInfo, nullptr, &frame.renderFence));
            VK_CHECK(vkCreateSemaphore(resource.device, &semaphoreCreateInfo, nullptr, &frame.swapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(resource.device, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore));
        }

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = resource.physicalDevice;
        allocatorInfo.device = resource.device;
        allocatorInfo.instance = resource.instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &resource.vmaAllocator);

        resource.globalAllocator = std::make_unique<DescriptorAllocator>();
        resource.globalAllocator->Init(resource.device);
    }

    void EngineResources::ImmediateSumbit(std::function<void(VkCommandBuffer cmd)>&& function) const
    {
        const VkCommandBuffer cmd = immCommandBuffer;

        auto cmdBeginInfo = vkinit::New<VkCommandBufferBeginInfo>(); {
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        }
        auto cmdinfo = vkinit::New<VkCommandBufferSubmitInfo>(); {
            cmdinfo.commandBuffer = cmd;
        }
        auto submit = vkinit::New<VkSubmitInfo2>(); {
            submit.pCommandBufferInfos = &cmdinfo;
            submit.commandBufferInfoCount = 1;
        }

        VK_CHECK(vkResetFences(resource.device, 1, &immFence));
        VK_CHECK(vkResetCommandBuffer(cmd, 0));
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        function(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));
        VK_CHECK(vkQueueSubmit2(resource.graphicsQueue, 1, &submit, immFence));
        VK_CHECK(vkWaitForFences(resource.device, 1, &immFence, true, 9999999999));
        // at this point, the submission has finished executing
    }

    Mesh EngineResources::UploadMesh(const std::span<uint32_t> indices, const std::span<Vertex> vertices) const
    {
        const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
        const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

        Mesh newSurface;

        //create vertex buffer
        newSurface.vertexBuffer = Buffer<Vertex>::Allocate(resource, vertices.size(),
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, HostAccess::NONE, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

        //find the adress of the vertex buffer
        const VkBufferDeviceAddressInfo deviceAdressInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer };
        newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(resource, &deviceAdressInfo);

        //create index buffer
        newSurface.indexBuffer = Buffer<uint32_t>::Allocate(resource, indices.size(),
                                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            HostAccess::NONE, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

        Buffer<std::byte> staging = Buffer<std::byte>::Allocate(resource, vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HostAccess::SEQUENTIAL_WRITE);

        std::memcpy(staging.Access(), vertices.data(), vertexBufferSize);
        std::memcpy(staging.Access() + vertexBufferSize, indices.data(), indexBufferSize);

        ImmediateSumbit([&](const VkCommandBuffer cmd)
        {
            VkBufferCopy vertexCopy;
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy;
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
        });

        staging.Destroy();

        return newSurface;
    }

    std::tuple<VkCommandBuffer, Image&> EngineResources::BeginFrame()
    {
        const FrameCommand& currentFrame = frames[frameCount % FRAME_OVERLAP];

        auto cmdBeginInfo = vkinit::New<VkCommandBufferBeginInfo>(); {
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        }

        // block until this frame has finished exeuting its last command
        VK_CHECK(vkWaitForFences(resource.device, 1, &currentFrame.renderFence, true, 1000000000));
        VK_CHECK(vkAcquireNextImageKHR(resource.device, swapchain, 1000000000, currentFrame.swapchainSemaphore, nullptr, &currentSwapchainImageIndex));
        VK_CHECK(vkResetFences(resource.device, 1, &currentFrame.renderFence));
        VK_CHECK(vkResetCommandBuffer(currentFrame.cmd, 0));
        VK_CHECK(vkBeginCommandBuffer(currentFrame.cmd, &cmdBeginInfo));

        return { currentFrame.cmd, swapchainImages[currentSwapchainImageIndex] };
    }

    void EngineResources::EndFrame()
    {
        const FrameCommand& currentFrame = frames[frameCount++ % FRAME_OVERLAP];

        VkMemoryBarrier2 barrier;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_NONE;

        // transition to present image
        swapchainImages[currentSwapchainImageIndex].Barrier(currentFrame.cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, barrier);

        auto cmdinfo = vkinit::New<VkCommandBufferSubmitInfo>(); {
            cmdinfo.commandBuffer = currentFrame.cmd;
        }

        auto waitInfo = vkinit::New<VkSemaphoreSubmitInfo>(); {
            waitInfo.semaphore = currentFrame.swapchainSemaphore;
            waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            waitInfo.value = 1;
        }

        auto signalInfo = vkinit::New<VkSemaphoreSubmitInfo>(); {
            signalInfo.semaphore = currentFrame.renderSemaphore;
            signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
            signalInfo.value = 1;
        }

        auto submit = vkinit::New<VkSubmitInfo2>(); {
            submit.pWaitSemaphoreInfos = &waitInfo;
            submit.pSignalSemaphoreInfos = &signalInfo;
            submit.pCommandBufferInfos = &cmdinfo;

            submit.waitSemaphoreInfoCount = 1;
            submit.signalSemaphoreInfoCount = 1;
            submit.commandBufferInfoCount = 1;
        }

        auto presentInfo = vkinit::New<VkPresentInfoKHR>(); {
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pWaitSemaphores = &currentFrame.renderSemaphore;
            presentInfo.pImageIndices = &currentSwapchainImageIndex;

            presentInfo.swapchainCount = 1;
            presentInfo.waitSemaphoreCount = 1;
        }

        // 1. prepare the submission to the queue.
        //      we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
        //      we will signal the _renderSemaphore, to signal that rendering has finished
        // 2. submit command buffer to the queue and execute it.
        //       _renderFence will now block until the graphic commands finish execution
        // 3. prepare present
        //      this will put the image we just rendered to into the visible window.
        //      we want to wait on the _renderSemaphore for that,
        //      as its necessary that drawing commands have finished before the image is displayed to the user
        VK_CHECK(vkEndCommandBuffer(currentFrame.cmd));
        VK_CHECK(vkQueueSubmit2(resource.graphicsQueue, 1, &submit, currentFrame.renderFence));
        VK_CHECK(vkQueuePresentKHR(resource.graphicsQueue, &presentInfo));
    }

    void EngineResources::Destroy()
    {
        vkDestroySwapchainKHR(resource.device, swapchain, nullptr);

        for (auto& image : swapchainImages)
            image.Destroy();

        vkDestroyCommandPool(resource.device, immCommandPool, nullptr);
        vkDestroyFence(resource.device, immFence, nullptr);

        for (const auto& frame : frames)
        {
            vkDestroyCommandPool(resource.device, frame.commandPool, nullptr);
            vkDestroyFence(resource.device, frame.renderFence, nullptr);
            vkDestroySemaphore(resource.device, frame.renderSemaphore, nullptr);
            vkDestroySemaphore(resource.device, frame.swapchainSemaphore, nullptr);
        }

        resource.Destroy();
    }
}

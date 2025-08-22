#pragma once

namespace vkinit
{
template <typename VkType> VkType New();
template <> inline VkApplicationInfo New<VkApplicationInfo>() { return { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO }; }
template <> inline VkInstanceCreateInfo New<VkInstanceCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO }; }
template <> inline VkDeviceQueueCreateInfo New<VkDeviceQueueCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; }
template <> inline VkDeviceCreateInfo New<VkDeviceCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO }; }
template <> inline VkSubmitInfo New<VkSubmitInfo>() { return { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO }; }
template <> inline VkMemoryAllocateInfo New<VkMemoryAllocateInfo>() { return { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; }
template <> inline VkMappedMemoryRange New<VkMappedMemoryRange>() { return { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE }; }
template <> inline VkBindSparseInfo New<VkBindSparseInfo>() { return { .sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO }; }
template <> inline VkFenceCreateInfo New<VkFenceCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO }; }
template <> inline VkSemaphoreCreateInfo New<VkSemaphoreCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; }
template <> inline VkEventCreateInfo New<VkEventCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO }; }
template <> inline VkQueryPoolCreateInfo New<VkQueryPoolCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO }; }
template <> inline VkBufferCreateInfo New<VkBufferCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; }
template <> inline VkBufferViewCreateInfo New<VkBufferViewCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO }; }
template <> inline VkImageCreateInfo New<VkImageCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO }; }
template <> inline VkImageViewCreateInfo New<VkImageViewCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO }; }
template <> inline VkShaderModuleCreateInfo New<VkShaderModuleCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO }; }
template <> inline VkPipelineCacheCreateInfo New<VkPipelineCacheCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO }; }
template <> inline VkPipelineShaderStageCreateInfo New<VkPipelineShaderStageCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }; }
template <> inline VkPipelineVertexInputStateCreateInfo New<VkPipelineVertexInputStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO }; }
template <> inline VkPipelineInputAssemblyStateCreateInfo New<VkPipelineInputAssemblyStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO }; }
template <> inline VkPipelineTessellationStateCreateInfo New<VkPipelineTessellationStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO }; }
template <> inline VkPipelineViewportStateCreateInfo New<VkPipelineViewportStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; }
template <> inline VkPipelineRasterizationStateCreateInfo New<VkPipelineRasterizationStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO }; }
template <> inline VkPipelineMultisampleStateCreateInfo New<VkPipelineMultisampleStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO }; }
template <> inline VkPipelineDepthStencilStateCreateInfo New<VkPipelineDepthStencilStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO }; }
template <> inline VkPipelineColorBlendStateCreateInfo New<VkPipelineColorBlendStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO }; }
template <> inline VkPipelineDynamicStateCreateInfo New<VkPipelineDynamicStateCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO }; }
template <> inline VkGraphicsPipelineCreateInfo New<VkGraphicsPipelineCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO }; }
template <> inline VkComputePipelineCreateInfo New<VkComputePipelineCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO }; }
template <> inline VkPipelineLayoutCreateInfo New<VkPipelineLayoutCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO }; }
template <> inline VkSamplerCreateInfo New<VkSamplerCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO }; }
template <> inline VkDescriptorSetLayoutCreateInfo New<VkDescriptorSetLayoutCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO }; }
template <> inline VkDescriptorPoolCreateInfo New<VkDescriptorPoolCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO }; }
template <> inline VkDescriptorSetAllocateInfo New<VkDescriptorSetAllocateInfo>() { return { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO }; }
template <> inline VkWriteDescriptorSet New<VkWriteDescriptorSet>() { return { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }; }
template <> inline VkCopyDescriptorSet New<VkCopyDescriptorSet>() { return { .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET }; }
template <> inline VkFramebufferCreateInfo New<VkFramebufferCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO }; }
template <> inline VkRenderPassCreateInfo New<VkRenderPassCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO }; }
template <> inline VkCommandPoolCreateInfo New<VkCommandPoolCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; }
template <> inline VkCommandBufferAllocateInfo New<VkCommandBufferAllocateInfo>() { return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; }
template <> inline VkCommandBufferInheritanceInfo New<VkCommandBufferInheritanceInfo>() { return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO }; }
template <> inline VkCommandBufferBeginInfo New<VkCommandBufferBeginInfo>() { return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; }
template <> inline VkRenderPassBeginInfo New<VkRenderPassBeginInfo>() { return { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO }; }
template <> inline VkBufferMemoryBarrier New<VkBufferMemoryBarrier>() { return { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER }; }
template <> inline VkImageMemoryBarrier New<VkImageMemoryBarrier>() { return { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }; }
template <> inline VkMemoryBarrier New<VkMemoryBarrier>() { return { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER }; }
template <> inline VkSemaphoreSubmitInfo New<VkSemaphoreSubmitInfo>() { return { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO }; }
template <> inline VkCommandBufferSubmitInfo New<VkCommandBufferSubmitInfo>() { return { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO }; }
template <> inline VkSubmitInfo2 New<VkSubmitInfo2>() { return { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2 }; }
template <> inline VkPresentInfoKHR New<VkPresentInfoKHR>() { return { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR }; }
template <> inline VkRenderingAttachmentInfo New<VkRenderingAttachmentInfo>() { return { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO }; }
template <> inline VkRenderingInfo New<VkRenderingInfo>() { return { .sType = VK_STRUCTURE_TYPE_RENDERING_INFO }; }
template <> inline VkPipelineRenderingCreateInfo New<VkPipelineRenderingCreateInfo>() { return { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO }; }
template <> inline VkBufferDeviceAddressInfo New<VkBufferDeviceAddressInfo>() { return { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO }; }
template <> inline VkImageMemoryBarrier2 New<VkImageMemoryBarrier2>() { return { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 }; }
template <> inline VkDependencyInfo New<VkDependencyInfo>() { return { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO }; }
template <> inline VkImageBlit2 New<VkImageBlit2>() { return { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2 }; }
template <> inline VkBlitImageInfo2 New<VkBlitImageInfo2>() { return { .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 }; }
template <> inline VkSemaphoreWaitInfo New<VkSemaphoreWaitInfo>() { return { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO }; }
}

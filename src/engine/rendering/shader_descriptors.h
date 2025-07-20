#pragma once
#include <vector>
#include <cassert>
#include "vk_types.h"
#include "vk_new.h"
#include "assets-system/AssetLookup.h"

// Auto-generated shader descriptor set layouts
// DO NOT EDIT MANUALLY

namespace shader_layouts
{
    inline DescriptorSetLayoutInfo CreateSetLayout(const VkDevice device, std::vector<VkDescriptorSetLayoutBinding>* bindings)
    {
        auto info = vkinit::New<VkDescriptorSetLayoutCreateInfo>();
        {
            info.pBindings = bindings->data();
            info.bindingCount = static_cast<uint32_t>(bindings->size());
        }

        VkDescriptorSetLayout set;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

        return {set, bindings};
    }

    namespace global
    {
        constexpr auto SceneData_binding = VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = 0x00000011,
        };

        inline std::vector bindings_0 = {SceneData_binding, };
        inline DescriptorSetLayoutInfo CreateSetLayout_0(const VkDevice device)
        {
            return CreateSetLayout(device, &bindings_0);
        }
    }

    namespace default_shader
    {
        constexpr auto pixel_asset = assets_system::lookup::SHAD_default_shader_ps;
        constexpr auto vertex_asset = assets_system::lookup::SHAD_default_shader_vs;
        constexpr auto GLTFMaterialData_binding = VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = 0x00000001,
        };
        constexpr auto colorTex_binding = VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = 0x00000010,
        };
        constexpr auto colorTexSampler_binding = VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = 0x00000010,
        };

        inline std::vector bindings_1 = {GLTFMaterialData_binding, colorTex_binding, colorTexSampler_binding, };
        inline DescriptorSetLayoutInfo CreateSetLayout_1(const VkDevice device)
        {
            return CreateSetLayout(device, &bindings_1);
        }
    }

    namespace unlit_shader
    {
        constexpr auto pixel_asset = assets_system::lookup::SHAD_unlit_shader_ps;
        constexpr auto vertex_asset = assets_system::lookup::SHAD_unlit_shader_vs;
        constexpr auto colorTex_binding = VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 1,
            .stageFlags = 0x00000010,
        };
        constexpr auto colorTexSampler_binding = VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = 0x00000010,
        };

        inline std::vector bindings_1 = {colorTex_binding, colorTexSampler_binding, };
        inline DescriptorSetLayoutInfo CreateSetLayout_1(const VkDevice device)
        {
            return CreateSetLayout(device, &bindings_1);
        }
    }

}
#include "GenerateShaderLookup.h"

#include <spirv_reflect.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <map>
#include <ranges>
#include <fmt/format.h>

#include "AssetFile.h"

namespace assets_system
{
    struct BindingData
    {
        uint32_t binding;
        SpvReflectDescriptorType descriptorType;
        SpvReflectShaderStageFlagBits stages;
        uint32_t descriptorCount;
        std::string name;
    };

    struct ShaderData
    {
        std::string name;
        std::map<std::string_view, std::string> names;
        std::map<uint32_t, std::vector<BindingData>> descriptorSets;
    };

    std::string_view GetVulkanDescriptorType(const SpvReflectDescriptorType type)
    {
        switch (type)
        {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return "VK_DESCRIPTOR_TYPE_SAMPLER";
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
            case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
            default: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        }
    }

    std::string_view GetVulkanStageFlagName(const SpvReflectShaderStageFlagBits stage)
    {
        switch (stage)
        {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return "vertex";
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return "geometry";
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return "pixel";
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return "compute";
            default: return "all_stages";
        }
    }

    std::string ReplaceSymbols(const std::string& string)
    {
        std::string result = string;
        std::ranges::transform(result, result.begin(), [](const char c)
        {
            return std::isalnum(c) ? c : '_';
        });

        // Ensure it doesn't start with a digit
        if (!result.empty() && std::isdigit(result[0]))
            result = "_" + result;

        // If result is empty, use a default name
        if (result.empty())
            result = "unnamed_shader";

        return result;
    }

    std::string SanitizeName(const std::string& name)
    {
        std::string result = name;
        if (result.ends_with("_vs") ||
            result.ends_with("_ps") ||
            result.ends_with("_cs") ||
            result.ends_with("_gs"))
            result.resize(result.size() - 3);

        return ReplaceSymbols(result);
    }

    std::string GenerateShaderNamespace(const ShaderData& shader)
    {
        std::string result;

        result += fmt::format("    namespace {}\n", shader.name);
        result += fmt::format("    {{\n");

        for (auto& [field, name] : shader.names)
            result += fmt::format("        constexpr auto {}_asset = assets_system::lookup::SHAD_{};\n", field, name);

        // Generate binding constants for this shader
        for (const auto& set : shader.descriptorSets | std::views::values)
        {
            for (const auto& binding : set)
            {
                std::string bindingName = binding.name.empty() ? fmt::format("_{}", binding.binding) : binding.name;
                bindingName = SanitizeName(bindingName);

                result += fmt::format("        constexpr auto {}_binding = VkDescriptorSetLayoutBinding{{\n", bindingName);
                result += fmt::format("            .binding = {},\n", binding.binding);
                result += fmt::format("            .descriptorType = {},\n", GetVulkanDescriptorType(binding.descriptorType));
                result += fmt::format("            .descriptorCount = {},\n", binding.descriptorCount);
                result += fmt::format("            .stageFlags = 0x{:08X},\n", static_cast<int>(binding.stages));
                result += fmt::format("        }};\n");
            }
        }

        // Generate layout creation function for this shader
        result += "\n";
        for (auto& i : shader.descriptorSets | std::views::keys)
        {
            result += fmt::format("        inline std::vector bindings_{} = {{", i);
            for (const auto& binding : shader.descriptorSets.at(i))
            {
                std::string bindingName = binding.name.empty() ? fmt::format("_{}", binding.binding) : binding.name;
                bindingName = SanitizeName(bindingName);

                result += fmt::format("{}_binding, ", bindingName);
            }
            result += fmt::format("}};\n");
            result += fmt::format("        inline DescriptorSetLayoutInfo CreateSetLayout_{}(const VkDevice device)\n", i);
            result += fmt::format("        {{\n");
            result += fmt::format("            return CreateSetLayout(device, &bindings_{});\n", i);
            result += fmt::format("        }}\n");
        }

        result += "    }\n\n";

        return result;
    }

    std::string GenerateHeader(const std::vector<ShaderData>& shaders)
    {
        std::string header = R"(#pragma once
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

)";

        // Generate a namespace for each shader
        for (const auto& shader : shaders)
            if (!shader.descriptorSets.empty())
                header += GenerateShaderNamespace(shader);

        return header + "}";
    }

    ShaderData ReflectShader(ShaderData& globalData, const std::filesystem::path& path)
    {
        AssetFile spirvAsset = AssetFile::Load(path.c_str());

        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(spirvAsset.blob.size(), spirvAsset.blob.data(), &module);

        if (result != SPV_REFLECT_RESULT_SUCCESS)
        {
            fmt::print("Failed to create reflection module for {}\n", path.filename().string());
            return {};
        }

        std::string name = ReplaceSymbols(path.stem().stem().string());

        ShaderData shaderData;
        shaderData.name = SanitizeName(name);
        shaderData.names[GetVulkanStageFlagName(module.shader_stage)] = name;
        SpvReflectShaderStageFlagBits stage = module.shader_stage;

        // Get descriptor sets
        uint32_t setCount = 0;
        result = spvReflectEnumerateDescriptorSets(&module, &setCount, nullptr);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        if (setCount > 0)
        {
            std::vector<SpvReflectDescriptorSet*> sets(setCount);
            result = spvReflectEnumerateDescriptorSets(&module, &setCount, sets.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            // Copy the data instead of storing pointers
            for (uint32_t i = 0; i < setCount; ++i)
            {
                const auto* set = sets[i];

                auto& descriptorSets = set->set == 0 ? globalData.descriptorSets[0] : shaderData.descriptorSets[set->set];

                descriptorSets.reserve(set->binding_count);

                for (uint32_t j = 0; j < set->binding_count; ++j)
                {
                    const auto* binding = set->bindings[j];

                    BindingData bindingData;
                    bindingData.binding = binding->binding;
                    bindingData.descriptorType = binding->descriptor_type;
                    bindingData.descriptorCount = 1;
                    if (binding->array.dims_count > 0)
                        bindingData.descriptorCount = binding->array.dims[0];

                    bindingData.name = binding->name ? binding->name : "";
                    bindingData.stages = stage;

                    descriptorSets.push_back(bindingData);
                }
            }
        }

        // Now it's safe to destroy the module since we copied all the data
        spvReflectDestroyShaderModule(&module);

        return shaderData;
    }

    void UnionShaderWith(ShaderData& a, const ShaderData& b)
    {
        assert(a.name == b.name);

        for (auto& [field, filename] : b.names)
            a.names[field] = filename;

        for (const auto& i : b.descriptorSets | std::views::keys)
        {
            std::vector<BindingData>& seta = a.descriptorSets.try_emplace(i).first->second;
            for (const BindingData& binding : b.descriptorSets.at(i))
            {
                const auto& j = std::ranges::find_if(seta, [&binding](const BindingData& data)
                {
                    // make sure they're exactly the same (except for the stage), otherwise don't try and merge
                    return data.binding == binding.binding &&
                           data.descriptorCount == binding.descriptorCount &&
                           data.descriptorType == binding.descriptorType &&
                           data.name == binding.name;
                });
                if (j == seta.end())
                    seta.push_back(binding);
                else
                    j->stages = static_cast<SpvReflectShaderStageFlagBits>(j->stages | binding.stages);
            }
        }
    }

    void UnionShader(ShaderData& a)
    {
        std::map<std::tuple<uint32_t, uint32_t, SpvReflectDescriptorType, std::string>, uint32_t> bindingGroups;
        for (auto& descriptorSet : a.descriptorSets | std::views::values)
        {
            std::vector<BindingData> originalBindings = descriptorSet;
            descriptorSet.clear();
            bindingGroups.clear();

            for (const auto& binding : originalBindings)
            {
                auto key = std::make_tuple(binding.binding, binding.descriptorCount, binding.descriptorType, binding.name);
                auto it = bindingGroups.find(key);
                if (it == bindingGroups.end())
                    bindingGroups[key] = binding.stages;
                else
                    it->second |= binding.stages;
            }

            for (const auto& [key, stages] : bindingGroups)
            {
                BindingData mergedBinding;
                mergedBinding.binding = std::get<0>(key);
                mergedBinding.descriptorCount = std::get<1>(key);
                mergedBinding.descriptorType = std::get<2>(key);
                mergedBinding.name = std::get<3>(key);
                mergedBinding.stages = static_cast<SpvReflectShaderStageFlagBits>(stages);

                descriptorSet.push_back(mergedBinding);
            }
        }
    }

    std::vector<ShaderData> UnionShaders(const std::vector<ShaderData>& shaders)
    {
        std::vector<ShaderData> merged {};

        for (const auto& shader : shaders)
        {
            const auto& i = std::ranges::find_if(merged, [&shader](const ShaderData& data) { return data.name == shader.name; });

            if (i == merged.end())
                merged.push_back(shader);
            else
                UnionShaderWith(*i, shader);
        }

        return merged;
    }

    bool GenerateMetadata(const char* shaderDir, const char* assetExt, const char* outputFile)
    {
        if (!std::filesystem::exists(shaderDir))
        {
            fmt::println("Directory does not exist: {}", shaderDir);
            return false;
        }

        // all in set 0 go into the global namespace, this is a convention.
        ShaderData globalData;
        globalData.name = "global";
        globalData.descriptorSets[0] = {};

        std::vector<ShaderData> shaders {};

        for (const auto& entry : std::filesystem::recursive_directory_iterator(shaderDir))
        {
            if (entry.is_regular_file() && entry.path().string().ends_with(assetExt))
            {
                auto shaderData = ReflectShader(globalData, entry.path());
                UnionShader(shaderData);
                shaders.push_back(std::move(shaderData));
            }
        }

        UnionShader(globalData);
        shaders.insert(shaders.begin(), std::move(globalData));

        shaders = UnionShaders(shaders);
        shaders = UnionShaders(shaders);

        if (shaders.empty())
        {
            fmt::println("No shaders with descriptor sets found.");
            return false;
        }

        const std::string headerCode = GenerateHeader(shaders);

        std::ofstream file(outputFile);
        if (file.is_open())
        {
            file << headerCode;
            file.close();
            fmt::println("\nGenerated header written to: {}", outputFile);
        } else
        {
            fmt::println("\nFailed to open output file: {}", outputFile);
            return false;
        }

        return true;
    }
}

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <map>
#include <ranges>
#include <fmt/core.h>
#include "spirv_reflect.h"
#include <vulkan/vulkan.h>

struct BindingData
{
    uint32_t binding;
    SpvReflectDescriptorType descriptorType;
    VkShaderStageFlags stages;
    uint32_t descriptorCount;
    std::string name;
};

struct ShaderData
{
    std::string name;
    std::map<std::string_view, std::tuple<std::string, std::string>> names;
    std::map<uint32_t, std::vector<BindingData>> descriptorSets;
};

std::vector<uint32_t> LoadSpirvFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error(fmt::format("Failed to open file: {}", path.string()));

    size_t fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    return buffer;
}

std::string_view GetVulkanDescriptorType(SpvReflectDescriptorType type)
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

VkShaderStageFlags GetVulkanStageFlags(SpvReflectShaderStageFlagBits stage)
{
    switch (stage)
    {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
    default: return VK_SHADER_STAGE_ALL;
    }
}

std::string_view GetVulkanStageFlagName(SpvReflectShaderStageFlagBits stage)
{
    switch (stage)
    {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return "vertex";
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "tesselation_control";
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "tesselation_evaluation";
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return "geometry";
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return "pixel";
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return "compute";
    default: return "all_stages";
    }
}

std::string SanitizeName(const std::string& name)
{
    std::string result = name;
    if (result.ends_with("_vs") ||
        result.ends_with("_ps") ||
        result.ends_with("_cs") ||
        result.ends_with("_gs") ||
        result.ends_with("_hs") ||
        result.ends_with("_ds"))
        result.resize(result.size() - 3);

    // Replace non-alphanumeric characters with underscores
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

std::string GenerateShaderNamespace(const ShaderData& shader)
{
    std::string result;

    result += fmt::format("    namespace {}\n", shader.name);
    result += fmt::format("    {{\n", shader.name);
    
    for (auto& [field, name] : shader.names)
        result += fmt::format("        constexpr auto {}_entry = \"{}\";\n", field, std::get<0>(name));
    for (auto& [field, name] : shader.names)
        result += fmt::format("        constexpr auto {}_filename = PROJECT_ROOT \"{}\";\n", field, std::get<1>(name));

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
            result += fmt::format("            .stageFlags = 0x{:08X},\n", binding.stages);
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
    const auto spirvData = LoadSpirvFile(path);

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(
        spirvData.size() * sizeof(uint32_t),
        spirvData.data(),
        &module
    );

    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        fmt::print("Failed to create reflection module for {}\n", path.filename().string());
        return {};
    }

    ShaderData shaderData;
    shaderData.name = SanitizeName(path.stem().string());
    shaderData.names[GetVulkanStageFlagName(module.shader_stage)] = {module.entry_points[0].name, path.string().substr(sizeof(PROJECT_ROOT) - 1)};
    VkShaderStageFlags stage = GetVulkanStageFlags(module.shader_stage);

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

        fmt::print("Processed shader: {} ({} descriptor sets)\n", shaderData.name, setCount);
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
                j->stages |= binding.stages;
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
            mergedBinding.stages = stages;
            
            descriptorSet.push_back(mergedBinding);
        }
    }
}

std::vector<ShaderData> UnionShaders(const std::vector<ShaderData>& shaders)
{
    std::vector<ShaderData> merged{};

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

int main()
{
    std::string shaderDir = PROJECT_ROOT "/shaders/spv/";
    std::string outputFile = PROJECT_ROOT "/src/engine/rendering/shader_descriptors.h";

    fmt::print("Reflecting shaders in: {}\n", shaderDir);
    fmt::print("Output file: {}\n", outputFile);

    if (!std::filesystem::exists(shaderDir))
    {
        fmt::print("Directory does not exist: {}\n", shaderDir);
        return 1;
    }

    // all in set 0 go into the global namespace, this is a convention.
    ShaderData globalData;
    globalData.name = "global";
    globalData.descriptorSets[0] = {};
    
    std::vector<ShaderData> shaders{};

    for (const auto& entry : std::filesystem::directory_iterator(shaderDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".spv")
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
        fmt::print("No shaders with descriptor sets found.\n");
        return 1;
    }

    const std::string headerCode = GenerateHeader(shaders);

    try
    {
        std::ofstream file(outputFile);
        if (file.is_open())
        {
            file << headerCode;
            file.close();
            fmt::print("\nGenerated header written to: {}\n", outputFile);
        }
        else
        {
            fmt::print("\nFailed to open output file: {}\n", outputFile);
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        fmt::print("\nError writing to file: {}\n", e.what());
        return 1;
    }

    return 0;
}

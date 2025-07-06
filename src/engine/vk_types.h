// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
//> intro
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <source_location>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
//< intro 

struct DescriptorSetLayoutInfo
{
    VkDescriptorSetLayout set;
    std::vector<VkDescriptorSetLayoutBinding>* bindings;
};

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};

//< mat_types
//> vbuf_types
struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

//< vbuf_types

//< node_types
//> intro
// Exception class for Vulkan errors
class VulkanException : public std::runtime_error
{
public:
    VulkanException(VkResult result, const std::string& message)
        : std::runtime_error(message), result_(result) {}

    VkResult result() const { return result_; }

private:
    VkResult result_;
};

// Enhanced VK_CHECK function with source location and better error handling
inline void vk_check_impl(VkResult result, const std::source_location& loc = std::source_location::current())
{
    if (result != VK_SUCCESS)
    {
        std::string error_msg = fmt::format(
            "Vulkan Error: {} \n"
            "  File: {}:{}\n"
            "  Function: {}\n",
            static_cast<int32_t>(result),
            loc.file_name(),
            loc.line(),
            loc.function_name()
        );

        fmt::print(stderr, "{}", error_msg);
        throw VulkanException(result, error_msg);
    }
}

#define VK_CHECK(expr) vk_check_impl((expr))
//< intro

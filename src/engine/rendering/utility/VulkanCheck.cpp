#include "VulkanCheck.h"
#include <stdexcept>

#include "fmt/base.h"

void vk_check_impl(const VkResult result, const std::source_location& loc)
{
    if (result != VK_SUCCESS)
    {
        fmt::println(stderr, "Vulkan Error: {} \n File {}:{} \n Function: {}", static_cast<int32_t>(result), loc.file_name(), loc.line(), loc.function_name());
        throw std::runtime_error("Vulkan Error see stderr for details.");
    }
}

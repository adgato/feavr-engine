#pragma once
#include <source_location>

// Enhanced VK_CHECK function with source location and better error handling
void vk_check_impl(VkResult result, const std::source_location& loc = std::source_location::current());

#define VK_CHECK(expr) vk_check_impl((expr))
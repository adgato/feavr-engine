// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <filesystem>

#include "rendering/Core.h"

void loadGltf(Core* core, std::string_view filePath);


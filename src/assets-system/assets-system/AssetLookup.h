#pragma once
#include "AssetID.h"

namespace assets_system::lookup
{
    constexpr AssetID SCNE_structure = { 28, 0 };
    constexpr AssetID SCNE_basicmesh = { 18, 0 };
    constexpr AssetID SHAD_unlit_shader_vs = { 17, 0 };
    constexpr AssetID SHAD_unlit_shader_ps = { 17, 1 };
    constexpr AssetID SHAD_default_shader_vs = { 16, 0 };
    constexpr AssetID SHAD_default_shader_ps = { 16, 1 };
}
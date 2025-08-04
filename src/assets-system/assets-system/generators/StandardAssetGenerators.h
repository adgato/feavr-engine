#pragma once
#include "IAssetGenerator.h"

MAKE_ASSET_GENERATOR("SHAD", ShaderAssetGenerator, ".hlsl");
MAKE_ASSET_GENERATOR("TEXT", TextAssetGenerator, ".txt");
MAKE_ASSET_GENERATOR("____", AssetAssetGenerator, ".asset");

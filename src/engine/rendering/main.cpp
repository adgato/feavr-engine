#include <filesystem>

#include "rendering/VulkanEngine.h"

#include "assets-system/generators/StandardAssetGenerators.h"

#include "Core.h"
#include "assets-system/AssetManager.h"
#include "ecs/Engine.h"
#include "ecs/EngineExtensions.h"
#include "generators/SceneAssetGenerator.h"

//#include <iostream>
//#include "mathx.h"
//using namespace mathx;


void RegisterAssetGenerators()
{
    AssetAssetGenerator::Register();
    ShaderAssetGenerator::Register();
    TextAssetGenerator::Register();
    SceneAssetGenerator::Register();
}

int main()
{
    RegisterAssetGenerators();
    assets_system::AssetManager::RefreshAssets();

    Core core;
    core.Init();
    while (core.Next()) {}
    core.Destroy();

    return 0;
}

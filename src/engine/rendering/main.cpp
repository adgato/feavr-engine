#include "assets-system/generators/StandardAssetGenerators.h"

#include "Core.h"
#include "assets-system/AssetManager.h"
#include "generators/SceneAssetGenerator.h"

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
    assets_system::AssetManager::RefreshAssets(true);

    Core core;
    core.Init();
    while (core.Next()) {}
    core.Destroy();

    return 0;
}

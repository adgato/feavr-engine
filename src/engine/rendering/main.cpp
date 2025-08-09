#include "rendering/VulkanEngine.h"

#include "assets-system/generators/StandardAssetGenerators.h"

#include "Core.h"
#include "assets-system/AssetManager.h"
#include "ecs/Engine.h"
#include "ecs/EngineAliases.h"
#include "ecs/EngineExtensions.h"
#include "generators/SceneAssetGenerator.h"

//#include <iostream>
//#include "mathx.h"
//using namespace mathx;


void ECSTestSave()
{
    ecs::Engine engine;
    engine.Add<int>(engine.New(), 1);

    assets_system::AssetFile ecsSave("ECSX", 0);

    serial::Stream m;
    m.InitWrite();
    ECS_SERIALIZE(m, engine, int);

    ecsSave.WriteToBlob(m);
    ecsSave.Save(PROJECT_ROOT"/test.ecs");
}

void ECSTestLoad()
{
    ecs::Engine engine;

    assets_system::AssetFile ecsLoad = assets_system::AssetFile::Load(PROJECT_ROOT"/test.ecs");

    serial::Stream m = ecsLoad.ReadFromBlob();
    ECS_SERIALIZE(m, engine, int);

    fmt::println("{}", engine.Get<int>(0));
}

void RegisterAssetGenerators()
{
    AssetAssetGenerator::Register();
    ShaderAssetGenerator::Register();
    TextAssetGenerator::Register();
    SceneAssetGenerator::Register();
}

int main()
{


    // RegisterAssetGenerators();
    //
    // assets_system::AssetManager::RefreshAssets();
    //
    ECSTestSave();
    ECSTestLoad();
    // Core core;
    // core.Init();
    // while (core.Next()) {}
    // core.Destroy();

    return 0;
}
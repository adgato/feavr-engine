#include "rendering/VulkanEngine.h"

#include "Core.h"
#include "assets-system/AssetManager.h"
#include "ecs/EngineAliases.h"

//#include <iostream>
//#include "mathx.h"
//using namespace mathx;


void ECSTestSave()
{
    ecs::MainEntityManager manager;

    const Cat cat1 { serial::array<char>::NewFromData("hello 1", sizeof("hello 1")) };
    const Cat cat2 { serial::array<char>::NewFromData("hello 2", sizeof("hello 2")) };
    const Cat cat3 { serial::array<char>::NewFromData("hello 3", sizeof("hello 3")) };

    manager.NewEntity<Cat>(cat1);
    manager.NewEntity<Cat>(cat2);
    manager.NewEntity<Cat>(cat3);

    assets_system::AssetFile ecsSave("ECSX", 0);

    serial::Stream m;
    m.InitWrite();
    manager.Serialize(m);

    ecsSave.WriteToBlob(m);
    ecsSave.Save(PROJECT_ROOT"/test.ecs");

    manager.Destroy();
}

void ECSTestLoad()
{
    ecs::MainEntityManager manager;

    assets_system::AssetFile ecsLoad = assets_system::AssetFile::Load(PROJECT_ROOT"/test.ecs");
    serial::Stream m = ecsLoad.ReadFromBlob();

    manager.Serialize(m);

    fmt::println("{}", manager.GetComponent<Cat>(0).catSound->data());
    fmt::println("{}", manager.GetComponent<Cat>(1).catSound->data());
    fmt::println("{}", manager.GetComponent<Cat>(2).catSound->data());

    manager.Destroy();
}

int main()
{
    //assets_system::AssetManager::RefreshAssets(true);

    ECSTestSave();
    ECSTestLoad();
    Core core;
    core.Init();
    while (core.Next()) {}
    core.Destroy();

    return 0;
}
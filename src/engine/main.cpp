#include <vk_engine.h>

#include "Core.h"
#include "ecs/Serial.h"
#include "ecs/Engine.h"

//#include <iostream>
//#include "mathx.h"
//using namespace mathx;



void ECSTestSave()
{
    ecs::EntityManager manager;

    const Cat cat1 { serial::array { "hello 1", sizeof("hello 1") } };
    const Cat cat2 { serial::array { "hello 2", sizeof("hello 2") } };
    const Cat cat3 { serial::array { "hello 3", sizeof("hello 3") } };

    manager.NewEntity<Cat>(cat1);
    manager.NewEntity<Cat>(cat2);
    manager.NewEntity<Cat>(cat3);

    ecs::Serial::Save(PROJECT_ROOT"/test.ecs", manager);
    manager.Destroy();
}


void ECSTestLoad()
{
    ecs::EntityManager manager;

    ecs::Serial::Load(PROJECT_ROOT"/test.ecs", manager);

    fmt::println("{}", manager.GetComponent<Cat>(0).catSound.data());
    fmt::println("{}", manager.GetComponent<Cat>(1).catSound.data());
    fmt::println("{}", manager.GetComponent<Cat>(2).catSound.data());

    manager.Destroy();
}

int main()
{
    ECSTestSave();
    ECSTestLoad();
    //Core core;
    //core.Init();
    //while (core.Next()) {}
    //core.Destroy();

    return 0;
}

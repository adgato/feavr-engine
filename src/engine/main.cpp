#include <vk_engine.h>

#include "Core.h"
#include "ecs/Engine.h"

 //#include <iostream>
//#include "mathx.h"
//using namespace mathx;

struct Cat
{
    char* sound;

    void Destroy()
    {
        fmt::println("Dying words: {}", sound);
        delete[] sound;
    }
};

void ECSTest()
{
    ecs::EntityManager manager;

    ecs::Entity e1 = manager.NewEntity<Cat>(Cat {new char[] {"Meow1"}});

    manager.RefreshComponents();
    

    manager.AddComponent<Cat>(e1, Cat {new char[] {"Meow2"}});
    manager.AddComponent<Cat>(e1, Cat {new char[] {"Meow3"}});
    manager.RemoveComponent<Cat>(e1);
    
    manager.RefreshComponents();
}

int main()
{
    //ECSTest();
    Core core;
    core.Init();
    while (core.Next()) {}
    core.Destroy();

    return 0;
}

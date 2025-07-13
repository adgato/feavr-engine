#pragma once
#include "Engine.h"
#include "serialisation/SerialManager.h"

namespace ecs
{
    class Serial
    {
    public:
        static void Load(const char* filePath, EntityManager& e);
        static void Save(const char* filePath, EntityManager& e);
    };
}

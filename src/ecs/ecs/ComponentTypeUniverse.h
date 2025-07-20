#pragma once
#include "serialisation/array.h"
#include "serialisation/SerialManager.h"
#include "glm/matrix.hpp"

template <typename... Ts>
struct TypeUniverse;

#define COMPONENT_TYPE_UNIVERSE(...) \
    using ComponentTypeUniverse = TypeUniverse<__VA_ARGS__>; \
    inline constexpr const char* ComponentTypeNames = #__VA_ARGS__;

struct Cat
{
    SERIALIZABLE(0, serial::array<char>) catSound;
    SERIALIZABLE(1, int) cat;

    void Serialize(serial::SerialManager& m)
    {
        m.SerializeComponent(catSound, cat);
    }

    void Destroy()
    {
        catSound->Destroy();
    }
};

namespace rendering
{
    struct Transform
    {
        glm::mat4 transform;
        static void Destroy() {}
        static void Serialize(serial::SerialManager&) {}
    };
}

namespace ecs
{
    using namespace rendering;
    COMPONENT_TYPE_UNIVERSE(Cat, Transform);
}

#pragma once
#include "SerialTypes.h"
#include "array.h"

template <typename... Ts>
struct TypeUniverse;

#define COMPONENT_TYPE_UNIVERSE(...) \
    using ComponentTypeUniverse = TypeUniverse<__VA_ARGS__>; \
    inline constexpr const char* ComponentTypeNames = #__VA_ARGS__; \

struct Cat
{
    ecs::array<char> catSound {};

    void Serialize(ecs::SerialManager* m)
    {
        m->SerializeArr(&catSound, 0);
    }

    void Destroy()
    {
        catSound.Destroy();
    }
};

namespace rendering
{
    struct Transform
    {
        glm::mat4 transform;
        static void Destroy() {}
        static void Serialize(ecs::SerialManager*) {}
    };
}

namespace ecs
{
    using namespace rendering;
    COMPONENT_TYPE_UNIVERSE(Cat, Transform);
}
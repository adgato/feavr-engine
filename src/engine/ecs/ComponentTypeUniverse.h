#pragma once

template <typename... Ts>
struct TypeUniverse;

namespace rendering
{
    struct Transform;
}
struct Cat;

using ComponentTypeUniverse = TypeUniverse<Cat, rendering::Transform>;

#pragma once
#include "ComponentTypeDefinitions.h"

#define MAKE_ALIAS(Alias, ...) \
    using Alias##EntityManager = EntityManager<__VA_ARGS__>;

namespace ecs
{
    template <typename... Components>
    class EntityManager;

    FOR_EACH_ECS_ALIAS(MAKE_ALIAS);
}

#undef MAKE_ALIAS

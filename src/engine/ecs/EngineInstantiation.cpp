// this seperate source file allows us to seperate which engines we want from what the engine is.
#include "rendering/pass-system/DefaultPass.h"
#include "rendering/pass-system/PassComponent.h"
#include "rendering/pass-system/SubMesh.h"
#include "ecs/ComponentTypeDefinitions.h"

#include "ecs/ComponentTypeInfo.cpp"
#include "ecs/EntityManager.cpp"

#define EXPLICITLY_INSTANTIATE(Alias, ...) \
    template class ecs::ComponentTypeInfo<__VA_ARGS__>; \
    template class ecs::EntityManager<__VA_ARGS__>;

#define SET_COMPONENT_NAMES(Alias, ...) \
    template <> const char* ecs::ComponentTypeInfo<__VA_ARGS__>::GetTypeNames() { return #__VA_ARGS__; }

FOR_EACH_ECS_ALIAS(SET_COMPONENT_NAMES);
FOR_EACH_ECS_ALIAS(EXPLICITLY_INSTANTIATE);

#undef EXPLICITLY_INSTANTIATE
#undef SET_COMPONENT_NAMES

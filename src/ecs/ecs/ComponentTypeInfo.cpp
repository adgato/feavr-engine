#include "ComponentTypeInfo.h"

namespace ecs
{
    template <typename... Ts>
    ComponentTypeInfo<Ts...> ComponentTypeInfo<Ts...>::Construct()
    {
        static_assert((ComponentType<Ts> && ...));
        ComponentTypeInfo c;
        size_t i = 0;
        ((c.typeInfo[i++] = { index_of_type_v<Ts, Ts...>, sizeof(Ts), alignof(Ts) }), ...);
        i = 0;
        ((c.destroyers[i++] = [](void* t) { static_cast<Ts*>(t)->Destroy(); }), ...);
        i = 0;
        ((c.serializers[i++] = [](void* t, serial::Stream& s) { static_cast<Ts*>(t)->Serialize(s); }), ...);
        return c;
    }

    template <typename... Ts>
    void ComponentTypeInfo<Ts...>::CopyDefault(void* dest, const TypeID type) const
    {
        CopyDefaultRec<0>(dest, type);
    }

    template <typename... Ts>
    const TypeInfo& ComponentTypeInfo<Ts...>::GetTypeInfo(TypeID type) const
    {
        return typeInfo[type];
    }

    template <typename... Ts>
    void ComponentTypeInfo<Ts...>::Destroy(void* data, TypeID type) const
    {
        destroyers[type](data);
    }

    template <typename... Ts>
    void ComponentTypeInfo<Ts...>::Serialize(serial::Stream& serialManager, void* data, TypeID type) const
    {
        serializers[type](data, serialManager);
    }
}

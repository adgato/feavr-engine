#pragma once
#include <bitset>
#include <cstring>
#include <functional>

#include "ComponentConcepts.h"

namespace ecs
{
    template <typename... Ts>
    class ComponentTypeInfo
    {
        std::array<TypeInfo, sizeof...(Ts)> typeInfo {};
        std::array<std::function<void(void*)>, sizeof...(Ts)> destroyers {};
        std::array<std::function<void(void*, serial::Stream&)>, sizeof...(Ts)> serializers {};
        std::tuple<Ts...> defaults {};

        template <size_t I>
        void CopyDefaultRec(void* dest, const TypeID type) const
        {
            if (type == I)
            {
                const auto& value = std::get<I>(defaults);
                std::memcpy(dest, &value, sizeof(value));
                return;
            }
            if constexpr (I + 1 < sizeof...(Ts))
            {
                CopyDefaultRec<I + 1>(dest, type);
            }
        }

    public:

        static ComponentTypeInfo Construct();

        static const char* GetTypeNames() { return "NOT SET!"; }

        void CopyDefault(void* dest, TypeID type) const;

        const TypeInfo& GetTypeInfo(TypeID type) const;

        void Destroy(void* data, TypeID type) const;

        void Serialize(serial::Stream& serialManager, void* data, TypeID type) const;
    };
}

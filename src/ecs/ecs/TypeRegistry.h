#pragma once
#include <cassert>
#include <cstdint>

#include "TypenameRegistry.h"
#include "serialisation/Stream.h"

namespace ecs
{
    using TypeID = uint16_t;
    constexpr TypeID BadMaxType = UINT16_MAX;
    constexpr uint BadMaxIndex = UINT32_MAX;

    class TypeRegistry
    {
        inline static TypeID nextid = 0;
        inline static std::vector<TypeInfo> info {};
        inline static std::vector<void(*)(void*)> destroyers {};
        inline static std::vector<void(*)(void*, serial::Stream&)> serializers {};
        inline static std::vector<void(*)(void*)> widgets {};
        inline static std::vector<void*> defaults {};

        template <typename T>
        struct TypeRegistrar
        {
            static TypeID Register()
            {
                info.emplace_back(NameOf<T>(), nextid++, sizeof(T), alignof(T));

                if constexpr (serial::IsSerialType<T>)
                    serializers.emplace_back([](void* t, serial::Stream& m) { static_cast<T*>(t)->Serialize(m); });
                else if constexpr (std::is_trivially_copyable_v<T>)
                    serializers.emplace_back([](void* t, serial::Stream& m)
                    {
                        if (m.reading)
                            *static_cast<T*>(t) = m.reader.Read<T>();
                        else
                            m.writer.Write<T>(*static_cast<T*>(t));
                    });
                else
                    serializers.emplace_back();

                if constexpr (serial::IsDestroyable<T>)
                    destroyers.emplace_back([](void* t) { static_cast<T*>(t)->Destroy(); });
                else
                    destroyers.emplace_back();

                if constexpr (serial::IsRenderable<T>)
                    widgets.emplace_back([](void* t) { static_cast<T*>(t)->Widget(); });
                else
                    widgets.emplace_back();

                if constexpr (std::is_default_constructible_v<T>)
                {
                    static T typeDefault {};
                    defaults.emplace_back(&typeDefault);
                }
                else
                    defaults.emplace_back();

                return info.back().type;
            }

            // if this field is accessed in the program, all type information is available before main is called.
            inline static TypeID id = Register();
        };

        static void AssertRegistered(const TypeID type)
        {
            assert(type < info.size() && "Unknown type, make sure to call ecs::TypeRegistry::GetID<Type>() somewhere that will be compiled.");
        }

    public:
        static TypeID RegisteredCount() { return info.size(); }

        template <typename T>
        static TypeID GetID()
        {
            return TypeRegistrar<T>::id;
        }

        static TypeInfo GetInfo(const TypeID type)
        {
            AssertRegistered(type);
            return info[type];
        }

        static void CopyDefault(const TypeID type, void* dest)
        {
            AssertRegistered(type);
            std::memcpy(dest, defaults[type], info[type].size);
        }

        static void Destroy(const TypeID type, void* data)
        {
            AssertRegistered(type);
            if (destroyers[type])
                destroyers[type](data);
        }

        static void Widget(const TypeID type, void* data)
        {
            AssertRegistered(type);
            if (widgets[type])
                widgets[type](data);
        }

        static void Serialize(const TypeID type, void* data, serial::Stream& m)
        {
            AssertRegistered(type);
            if (serializers[type])
                serializers[type](data, m);
        }
    };
}

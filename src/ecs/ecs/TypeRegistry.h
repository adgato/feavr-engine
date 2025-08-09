#pragma once
#include <atomic>
#include <cassert>
#include <cstdint>

#include "serialisation/Stream.h"

namespace ecs
{
    using TypeID = uint16_t;
    constexpr TypeID BadMaxType = UINT16_MAX;
    constexpr uint BadMaxIndex = UINT32_MAX;

    class TypeRegistry
    {
        inline static std::atomic<TypeID> id = 0;

        inline static std::vector<TypeInfo> info {};
        inline static std::vector<void(*)(void*)> destroyers {};
        inline static std::vector<void(*)(void*, serial::Stream&)> serializers {};
        inline static std::vector<void*> defaults {};

    public:
        static TypeID RegisteredCount() { return info.size(); }

        template <typename>
        static TypeID GetID()
        {
            static TypeID typeID = id++;
            return typeID;
        }

        template <typename T>
        static void Register()
        {
            const TypeID type = TypeRegistry::GetID<T>();

            if (type >= info.size())
            {
                while (info.size() <= type)
                    info.emplace_back(info.size(), 0, 0);
                destroyers.resize(type + 1);
                serializers.resize(type + 1);
                defaults.resize(type + 1);
            } else if (info[type].size > 0)
                return;

            info[type] = { type, sizeof(T), alignof(T) };

            static T typeDefault {};
            defaults[type] = &typeDefault;

            if constexpr (serial::IsDestroyable<T>)
                destroyers[type] = [](void* t) { static_cast<T*>(t)->Destroy(); };

            if constexpr (serial::IsSerialType<T>)
                serializers[type] = [](void* t, serial::Stream& m) { static_cast<T*>(t)->Serialize(m); };
            else if constexpr (std::is_trivially_copyable_v<T>)
                serializers[type] = [](void* t, serial::Stream& m)
                {
                    if (m.reading)
                        *static_cast<T*>(t) = m.reader.Read<T>();
                    else
                        m.writer.Write<T>(*static_cast<T*>(t));
                };
        }

        static TypeInfo GetInfo(const TypeID type) { return info[type]; }

        static void CopyDefault(const TypeID type, void* dest) { std::memcpy(dest, defaults[type], info[type].size); }

        static void Destroy(const TypeID type, void* data)
        {
            if (destroyers[type])
                destroyers[type](data);
        }

        static void Serialize(const TypeID type, void* data, serial::Stream& m)
        {
            if (serializers[type])
                serializers[type](data, m);
        }
    };
}

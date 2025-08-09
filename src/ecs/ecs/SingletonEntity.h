#pragma once
#include <functional>
#include <memory>
#include <tuple>
#include <vector>

#include "serialisation/Stream.h"
#include "ComponentConcepts.h"
#include "SingletonView.h"

#define SINGLETON_ENTITY(...) ecs::SingletonEntity<__VA_ARGS__>(#__VA_ARGS__)

namespace ecs
{
    template <IsSerialTypeAndDestroyable... Components>
    class SingletonEntity
    {
        static constexpr size_t NumTypes = sizeof...(Components);
        std::tuple<Components...> components {};
        const char* componentNames;

        static std::vector<size_t> split(std::string&& s, const std::string& delimiter)
        {
            size_t pos_start = 0;
            size_t pos_end;
            const size_t delim_len = delimiter.length();
            std::vector<size_t> res {};

            if (s.ends_with('\0'))
                s.erase(s.find_first_of('\0'));

            while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
            {
                std::string token = s.substr(pos_start, pos_end - pos_start);
                pos_start = pos_end + delim_len;
                res.push_back(std::hash<std::string> {}(token));
            }

            res.push_back(std::hash<std::string> {}(s.substr(pos_start)));
            return res;
        }

    public:
        explicit SingletonEntity(const char* componentNames) : componentNames(componentNames) {}

        template <typename T> requires ecs::one_of_v<T, Components...>
        T& Get()
        {
            return std::get<T>(components);
        }

        template <typename... Ts> requires (ecs::one_of_v<Ts, Components...> && ...)
        SingletonView<Ts...> View()
        {
            SingletonView<Ts...> view;
            view.Init(&std::get<Ts>(components)...);
            return view;
        }

        void Serialize(serial::Stream& m)
        {
            static std::array<std::function<void(std::tuple<Components...>&, serial::Stream&)>, NumTypes> serializers
            {
                [](std::tuple<Components...>& c, serial::Stream& s) { std::get<Components>(c).Serialize(s); }...
            };

            // similar implementation to that of EntityManager
            if (m.reading)
            {
                const auto targetSplit = split(m.reader.ReadString(), ", ");
                const auto sourceSplit = split(componentNames, ", ");

                const uint targetNumTypes = targetSplit.size();
                assert(sourceSplit.size() == NumTypes);

                std::vector remap(targetNumTypes, NumTypes);
                for (size_t i = 0; i < targetSplit.size(); ++i)
                    for (size_t j = 0; j < NumTypes; ++j)
                        if (targetSplit[i] == sourceSplit[j])
                        {
                            remap[i] = j;
                            break;
                        }

                for (uint i = 0; i < targetNumTypes; ++i)
                {
                    const size_t size = m.reader.Read<serial::fsize>();
                    const TypeID type = remap[i];
                    if (type >= NumTypes)
                        m.reader.Jump(size);
                    else
                        serializers[type](components, m);
                }
            } else
            {
                m.writer.WriteString(componentNames);
                size_t offset = 0;
                ((offset = m.writer.Reserve<serial::fsize>(),
                  std::get<Components>(components).Serialize(m),
                  m.writer.WriteOver<serial::fsize>(m.writer.GetCount() - offset - sizeof(serial::fsize), offset)
                ), ...);
            }
        }

        void Destroy()
        {
            (std::get<Components>(components).Destroy(), ...);
        }
    };
}

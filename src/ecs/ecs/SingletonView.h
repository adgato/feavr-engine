#pragma once
#include <tuple>

#include "ComponentConcepts.h"

namespace ecs
{
    template <typename... Components>
    class SingletonView
    {
        std::tuple<Components*...> components {};

    public:
        void Init(Components*... components)
        {
            this->components = std::tuple<Components*...>(components...);
        }

        template <typename T> requires ecs::one_of_v<T, Components...>
        T& Get()
        {
            return *std::get<T*>(components);
        }

        template <typename... Ts> requires (ecs::one_of_v<Ts, Components...> && ...)
        SingletonView<Ts...> CreateView()
        {
            SingletonView<Ts...> view;
            view.Init(std::get<Ts*>(components)...);
            return view;

        }
    };
}

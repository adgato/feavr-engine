#pragma once
#include "PassManager.h"

namespace rendering
{
    template<typename T, typename... Ts>
    constexpr std::size_t index_of_type()
    {
        std::size_t index = 0;
        ((std::is_same_v<T, Ts> ? true : (++index, false)) || ...);
        return index;
    }

    template <typename... Passes> requires (std::derived_from<Passes, passes::Pass> && ...)
    class Material
    {
        PassManager* manager = nullptr;
        std::array<uint32_t, sizeof...(Passes)> instanceIdxs;
    public:

        // Create a material with new instances of every pass
        static Material New(PassManager* passManager)
        {
            Material material;
            material.manager = passManager;
            material.instanceIdxs = { passManager->AddPassInstance<Passes>()... };
            return material;
        }

        // Create a material using the first existing instance of every pass, creating new instances as needed
        static Material First(PassManager* passManager)
        {
            Material material;
            material.manager = passManager;
            material.instanceIdxs = { (passManager->GetPassCount<Passes>() > 0 ? 0 : passManager->AddPassInstance<Passes>())... };
            return material;
        }

        // Create a material using the specified instances of every pass
        static Material Inherit(PassManager* passManager, std::array<uint32_t, sizeof...(Passes)> instanceIdxs)
        {
            Material material;
            material.manager = passManager;
            material.instanceIdxs = instanceIdxs;
            return material;
        }

        template <typename T> requires (std::same_as<T, Passes> || ...)
        uint32_t GetInstanceIdx()
        {
            return instanceIdxs[index_of_type<T, Passes...>()];
        }
        
        template <typename T> requires (std::same_as<T, Passes> || ...)
        passes::PassInst<T>& GetInstance()
        {
            return manager->GetPassInstance<T>(GetInstanceIdx<T>());
        }
        
        void AddMesh(uint32_t meshIdx)
        {
            (GetInstance<Passes>().meshIdxs.push_back(meshIdx), ...);
        }
    };
}

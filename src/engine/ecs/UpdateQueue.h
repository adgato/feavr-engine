#pragma once
#include <type_traits>

#include "Common.h"

namespace ecs
{
    
    struct UpdateInstr
    {
        uint order;
        std::unique_ptr<std::byte[]> data;

        template <ComponentType T>
        static UpdateInstr NewAdd(const T& data, const uint order)
        {
            UpdateInstr instr;

            instr.order = order;
            instr.data = std::make_unique<std::byte[]>(sizeof(T));
        
            std::memcpy(instr.data.get(), &data, sizeof(T));
        
            return instr;
        }

        static UpdateInstr NewRemove(const uint order)
        {
            UpdateInstr instr;

            instr.order = order;
            instr.data = nullptr;
        
            return instr;
        }
    };
}

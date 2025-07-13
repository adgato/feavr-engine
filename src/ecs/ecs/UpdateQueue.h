#pragma once

#include "Common.h"
#include <memory>

namespace ecs
{
    struct UpdateInstr
    {
        TypeID type;
        std::unique_ptr<std::byte[]> data;

        template <ComponentType T>
        static UpdateInstr NewAdd(const T& data, const TypeID type)
        {
            UpdateInstr instr;

            instr.type = type;
            instr.data = std::make_unique<std::byte[]>(sizeof(T));
        
            std::memcpy(instr.data.get(), &data, sizeof(T));
        
            return instr;
        }

        static UpdateInstr NewRawAdd(const std::byte* data, const TypeID type)
        {
            UpdateInstr instr;

            const size_t size = ComponentInfo.GetTypeInfo(type).size;

            instr.type = type;
            instr.data = std::make_unique<std::byte[]>(size);

            std::memcpy(instr.data.get(), data, size);

            return instr;
        }

        static UpdateInstr NewRemove(const TypeID type)
        {
            UpdateInstr instr;

            instr.type = type;
            instr.data = nullptr;
        
            return instr;
        }
    };
}

#pragma once

#include <memory>
#include "ComponentConcepts.h"
#include "TypeRegistry.h"

namespace ecs
{
    struct UpdateInstr
    {
        TypeID type;
        std::unique_ptr<std::byte[]> data;

        template <ComponentType T>
        static UpdateInstr Add(const T& data, const TypeID type)
        {
            UpdateInstr instr;

            instr.type = type;
            instr.data = std::make_unique<std::byte[]>(sizeof(T));
        
            std::memcpy(instr.data.get(), &data, sizeof(T));
        
            return instr;
        }

        static UpdateInstr RawAdd(const std::byte* data, const TypeID type, const size_t size)
        {
            UpdateInstr instr;

            instr.type = type;
            instr.data = std::make_unique<std::byte[]>(size);

            std::memcpy(instr.data.get(), data, size);

            return instr;
        }

        static UpdateInstr Remove(const TypeID type)
        {
            return { type, nullptr };
        }
    };
}

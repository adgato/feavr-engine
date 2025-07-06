#pragma once
#include "rendering/shader_descriptors.h"
#include "DefaultPass.h"

namespace rendering::passes
{
    namespace default_pass
    {
        using Pass = DefaultPass;
        using Instance = PassInst<Pass>;
        using namespace shader_layouts::default_shader;
    }
}

namespace default_pass = rendering::passes::default_pass;

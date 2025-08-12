#pragma once
#include "shader_descriptors.h"

namespace rendering::passes
{
    class DefaultPass;
    class IdentifyPass;

    template <typename>
    struct PassComponent;

    namespace default_pass
    {
        using Pass = DefaultPass;
        using Component = PassComponent<Pass>;
        using namespace shader_layouts::default_shader;
    }
    namespace unlit_pass
    {
        using Pass = IdentifyPass;
        using Component = PassComponent<Pass>;
    }
}

namespace default_pass = rendering::passes::default_pass;
namespace unlit_pass = rendering::passes::unlit_pass;
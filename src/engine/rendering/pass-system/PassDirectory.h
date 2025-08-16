#pragma once
#include "shader_descriptors.h"

namespace rendering::passes
{
    class StencilOutlinePass;
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
    namespace identify_pass
    {
        using Pass = IdentifyPass;
        using Component = PassComponent<Pass>;
    }
    namespace stencil_outline_pass
    {
        using Pass = StencilOutlinePass;
        using Component = PassComponent<Pass>;
    }
}

namespace default_pass = rendering::passes::default_pass;
namespace identify_pass = rendering::passes::identify_pass;
namespace stencil_outline_pass = rendering::passes::stencil_outline_pass;
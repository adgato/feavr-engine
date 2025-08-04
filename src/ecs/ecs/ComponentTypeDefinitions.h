#pragma once
#include "serialisation/array.h"
#include "glm/gtc/type_ptr.hpp"

namespace rendering
{
    struct SubMesh;

    namespace passes
    {
        template <typename>
        struct PassComponent;
        class DefaultPass;
    }
}

#define FOR_EACH_ECS_ALIAS(MACRO) \
    MACRO(Main, Cat, rendering::Transform) \
    MACRO(Pass, rendering::SubMesh, rendering::passes::PassComponent<rendering::passes::DefaultPass>)

// example usage


struct Cat
{
    SERIALIZABLE(0, serial::array<char>) catSound;
    SERIALIZABLE(1, int) cat;

    void Serialize(serial::Stream& m)
    {
        m.SerializeComponent(catSound, cat);
    }

    void Destroy()
    {
        catSound->Destroy();
    }
};

namespace rendering
{
    struct Transform
    {
        glm::mat4 transform;
        static void Destroy() {}
        void Serialize(serial::Stream& m)
        {
            m.SerializeArray<float>(glm::value_ptr(transform), 16);
        }
    };
}
#pragma once
#include "glm/mat4x4.hpp"

namespace serial
{
    class Stream;
}

struct Transform
{
    glm::mat4 transform;

    void Serialize(serial::Stream& m);
    void Widget();
};

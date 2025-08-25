#pragma once
#include "glm/mat4x4.hpp"

namespace serial
{
    class Stream;
}

struct Transform
{
private:
    glm::mat4 transform {};

public:
    bool hasChanged = false;

    glm::mat4& ModifyMatrix()
    {
        hasChanged = true;
        return transform;
    };

    const glm::mat4& Matrix() const
    {
        return transform;
    }

    void Serialize(serial::Stream& m);

    void Widget();
};

#pragma once
#include "glm/vec3.hpp"
#include "glm/gtx/quaternion.hpp"
#include "serialisation/Stream.h"

namespace serial
{
    class Stream;
}

struct Transform
{
private:
    SERIALIZABLE(0, glm::vec3) position {};
    SERIALIZABLE(1, glm::quat) rotation {};
    SERIALIZABLE(2, glm::vec3) scale {};

public:
    bool hasChanged = true;

    glm::vec3 TransformPoint(glm::vec3 p) const
    {
        p *= *scale;
        p = *rotation * p;
        p += *position;
        return p;
    }

    glm::vec3 TransformVector(glm::vec3 v) const
    {
        v *= *scale;
        v = *rotation * v;
        return v;
    }

    glm::vec3 TransformDirection(glm::vec3 d) const
    {
        d = *rotation * d;
        return d;
    }

    glm::vec3& SetPosition()
    {
        hasChanged = true;
        return position;
    }

    const glm::vec3& Position() const
    {
        return position;
    }

    glm::quat& SetRotation()
    {
        hasChanged = true;
        return rotation;
    }

    const glm::quat& Rotation() const
    {
        return rotation;
    }

    glm::vec3& SetScale()
    {
        hasChanged = true;
        return scale;
    }

    const glm::vec3& Scale() const
    {
        return scale;
    }

    void Serialize(serial::Stream& m);

    void Widget();
};

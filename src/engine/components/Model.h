#pragma once
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "ecs/EntityRef.h"

namespace ecs {
    class Engine;
}

class Camera;
struct Mesh;
struct Transform;

struct Model
{
    SERIALIZABLE(0, ecs::EntityRef) meshRef;
    SERIALIZABLE(1, bool) active = { true };
    glm::mat4 transform;
    glm::vec3 minAABB;
    glm::vec3 maxAABB;
    bool visible = true;

    void Update(const ecs::Engine& engine, const Transform& tf);
    void RefreshCull(const Camera& camera);

    void Serialize(serial::Stream& m)
    {
        m.SerializeComponent(meshRef, active);
    }

    void Widget();
};
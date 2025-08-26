#pragma once

#include <SDL_events.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/quaternion.hpp"


class Camera
{
public:
    glm::vec3 velocity;
    glm::vec3 position;
    // vertical rotation
    float pitch { 0.f };
    // horizontal rotation
    float yaw { 0.f };

    float fovy = glm::radians(80.f);
    float near = 10000.f;
    float far = 0.1f;
    float aspect = 1;

    enum { LEFT, RIGHT, BOTTOM, TOP, NEAR, FAR };

    glm::vec4 frustumPlanes[6];

    glm::mat4 view;
    glm::mat4 proj;

    bool enableMotion = false;

    void SetAspect(const float aspect)
    {
        this->aspect = aspect;
    }

    void processSDLEvent(SDL_Event& e);

    void Update();
};

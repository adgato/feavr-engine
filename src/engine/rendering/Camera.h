#pragma once

#include <SDL_events.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    // vertical rotation
    float pitch { 0.f };
    // horizontal rotation
    float yaw { 0.f };

    bool enableMotion = false;

    glm::mat4 getViewMatrix();
    glm::mat4 getRotationMatrix();

    void processSDLEvent(SDL_Event& e);

    void update();
};
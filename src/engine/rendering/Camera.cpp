#include "Camera.h"


void Camera::Update()
{
    const glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    const glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });
    const glm::quat invPitchRotation = glm::quat(pitchRotation.w, -pitchRotation.x, -pitchRotation.y, -pitchRotation.z);
    const glm::quat invYawRotation = glm::quat(yawRotation.w, -yawRotation.x, -yawRotation.y, -yawRotation.z);

    const glm::quat rotation = yawRotation * pitchRotation;
    const glm::quat invRotation = invPitchRotation * invYawRotation;

    position += rotation * velocity * 0.5f;


    glm::vec3 right = rotation * glm::vec3(1, 0, 0);
    glm::vec3 up = rotation * glm::vec3(0, 1, 0);
    glm::vec3 forward = rotation * glm::vec3(0, 0, -1);

    const float halfVSide = far * tanf(fovy * .5f);
    const float halfHSide = halfVSide * aspect;
    const glm::vec3 frontMultFar = far * forward;

    // Determine which is actually closer/farther
    frustumPlanes[NEAR] = glm::vec4(forward, -glm::dot(forward, position + glm::min(near, far) * forward));
    frustumPlanes[FAR] = glm::vec4(-forward, -glm::dot(-forward, position + glm::max(near, far) * forward));

    // Side planes (matching the reference cross products)
    glm::vec3 rightNormal = glm::normalize(glm::cross(frontMultFar - right * halfHSide, up));
    glm::vec3 leftNormal = glm::normalize(glm::cross(up, frontMultFar + right * halfHSide));
    glm::vec3 topNormal = glm::normalize(glm::cross(right, frontMultFar - up * halfVSide));
    glm::vec3 bottomNormal = glm::normalize(glm::cross(frontMultFar + up * halfVSide, right));

    frustumPlanes[RIGHT] = glm::vec4(rightNormal, -glm::dot(rightNormal, position));
    frustumPlanes[LEFT] = glm::vec4(leftNormal, -glm::dot(leftNormal, position));
    frustumPlanes[TOP] = glm::vec4(topNormal, -glm::dot(topNormal, position));
    frustumPlanes[BOTTOM] = glm::vec4(bottomNormal, -glm::dot(bottomNormal, position));

    view = glm::toMat4(invRotation) * glm::translate(glm::mat4(1.f), -position);
    proj = glm::perspective(fovy, aspect, near, far);
    proj[1][1] *= -1;
}

void Camera::processSDLEvent(SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN)
    {
        if (e.key.keysym.sym == SDLK_w) { velocity.z = -1; }
        if (e.key.keysym.sym == SDLK_s) { velocity.z = 1; }
        if (e.key.keysym.sym == SDLK_a) { velocity.x = -1; }
        if (e.key.keysym.sym == SDLK_d) { velocity.x = 1; }
    }

    if (e.type == SDL_KEYUP)
    {
        if (e.key.keysym.sym == SDLK_w) { velocity.z = 0; }
        if (e.key.keysym.sym == SDLK_s) { velocity.z = 0; }
        if (e.key.keysym.sym == SDLK_a) { velocity.x = 0; }
        if (e.key.keysym.sym == SDLK_d) { velocity.x = 0; }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
        enableMotion = true;
    else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
        enableMotion = false;

    if (enableMotion && e.type == SDL_MOUSEMOTION)
    {
        yaw += (float)e.motion.xrel / 200.f;
        pitch -= (float)e.motion.yrel / 200.f;
    }
}

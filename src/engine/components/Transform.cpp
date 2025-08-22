#include "Transform.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/quaternion.hpp"
#include "serialisation/Stream.h"

void Transform::Serialize(serial::Stream& m)
{
    m.SerializeArray<float>(glm::value_ptr(transform), 16);
}

void Transform::Widget()
{
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(transform, scale, rotation, translation, skew, perspective);
    rotation = glm::normalize(rotation);

    // Get current euler angles for display
    glm::vec3 displayEuler = glm::degrees(glm::eulerAngles(rotation));
    for (int i = 0; i < 3; i++)
    {
        while (displayEuler[i] > 180.0f) displayEuler[i] -= 360.0f;
        while (displayEuler[i] < -180.0f) displayEuler[i] += 360.0f;
    }

    bool changed = false;

    if (ImGui::DragFloat3("Position", glm::value_ptr(translation), 0.05f))
        changed = true;

    glm::vec3 editEuler = displayEuler;
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(editEuler), 1.0f, 0, 0, "%.1f°"))
    {
        rotation *= glm::quat(glm::radians(editEuler - displayEuler));
        rotation = glm::normalize(rotation);
        changed = true;
    }

    if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.005f, 0.001f, 100.0f))
        changed = true;

    if (changed)
    {
        transform = glm::translate(glm::mat4(1.0f), translation) *
                    glm::toMat4(rotation) *
                    glm::scale(glm::mat4(1.0f), glm::max(glm::vec3(0.001f, 0.001f, 0.001f), scale));
    }
}

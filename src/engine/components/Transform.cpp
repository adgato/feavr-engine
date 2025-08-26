#include "Transform.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"
#include "serialisation/Stream.h"

void Transform::Serialize(serial::Stream& m)
{
    m.SerializeComponent(position, rotation, scale);
}

void Transform::Widget()
{
    // Get current euler angles for display
    glm::vec3 displayEuler = glm::degrees(glm::eulerAngles(*rotation));
    for (int i = 0; i < 3; i++)
    {
        while (displayEuler[i] > 180.0f) displayEuler[i] -= 360.0f;
        while (displayEuler[i] < -180.0f) displayEuler[i] += 360.0f;
    }

    hasChanged |= ImGui::DragFloat3("Position", glm::value_ptr(*position), 0.05f);

    glm::vec3 editEuler = displayEuler;
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(editEuler), 1.0f, 0, 0, "%.1f°"))
    {
        *rotation *= glm::quat(glm::radians(editEuler - displayEuler));
        *rotation = glm::normalize(*rotation);
        hasChanged = true;
    }

    hasChanged |= ImGui::DragFloat3("Scale", glm::value_ptr(*scale), 0.005f, 0.001f, 100.0f);
}

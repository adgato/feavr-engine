#include "Model.h"

#include "imgui.h"
#include "Transform.h"
#include "ecs/Engine.h"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "rendering/Camera.h"
#include "rendering/pass-system/Mesh.h"

void Model::Update(const ecs::Engine& engine, const Transform& tf)
{
    transform = glm::translate(glm::mat4(1.0f), tf.Position()) *
                glm::toMat4(tf.Rotation()) *
                glm::scale(glm::mat4(1.0f), tf.Scale());

    const Mesh& mesh = engine.Get<Mesh>(meshRef->id);
    const glm::vec3 corners[8] = {
        { mesh.boundsMin.x, mesh.boundsMin.y, mesh.boundsMin.z }, { mesh.boundsMax.x, mesh.boundsMin.y, mesh.boundsMin.z },
        { mesh.boundsMin.x, mesh.boundsMax.y, mesh.boundsMin.z }, { mesh.boundsMax.x, mesh.boundsMax.y, mesh.boundsMin.z },
        { mesh.boundsMin.x, mesh.boundsMin.y, mesh.boundsMax.z }, { mesh.boundsMax.x, mesh.boundsMin.y, mesh.boundsMax.z },
        { mesh.boundsMin.x, mesh.boundsMax.y, mesh.boundsMax.z }, { mesh.boundsMax.x, mesh.boundsMax.y, mesh.boundsMax.z }
    };

    // Transform all corners
    minAABB = tf.TransformVector(corners[0]);
    maxAABB = minAABB;

    for (int i = 1; i < 8; i++)
    {
        glm::vec3 worldCorner = tf.TransformVector(corners[i]);
        minAABB = glm::min(minAABB, worldCorner);
        maxAABB = glm::max(maxAABB, worldCorner);
    }

    minAABB += tf.Position();
    maxAABB += tf.Position();
}

void Model::RefreshCull(const Camera& camera)
{
    visible = active;
    if (!visible)
        return;

    for (size_t i = 0; i < 6; ++i)
    {
        const glm::vec4& plane = camera.frustumPlanes[i];

        // Find the "positive vertex" (corner furthest along plane normal)
        glm::vec3 useMax = glm::step(glm::vec3 { 0 }, glm::vec3 { plane });
        glm::vec4 positiveVertex = glm::vec4 { glm::mix(minAABB, maxAABB, useMax), 1 };

        // If positive vertex is behind plane, entire AABB is outside
        if (glm::dot(plane, positiveVertex) < 0)
        {
            visible = false;
            return;
        }
    }
}

void Model::Widget()
{
    ImGui::Checkbox("Active", &*active);
    ImGui::BeginDisabled();
    ImGui::Checkbox("Visible", &visible);
    ImGui::InputScalar("Mesh ID", ImGuiDataType_U32, &meshRef->id);
    ImGui::InputFloat3("Min AABB", glm::value_ptr(minAABB));
    ImGui::InputFloat3("Max AABB", glm::value_ptr(maxAABB));
    ImGui::SeparatorText("Transform");

    glm::vec4 row0 = glm::row(transform, 0);
    glm::vec4 row1 = glm::row(transform, 1);
    glm::vec4 row2 = glm::row(transform, 2);
    glm::vec4 row3 = glm::row(transform, 3);
    ImGui::InputFloat4("Row 0", glm::value_ptr(row0));
    ImGui::InputFloat4("Row 1", glm::value_ptr(row1));
    ImGui::InputFloat4("Row 2", glm::value_ptr(row2));
    ImGui::InputFloat4("Row 3", glm::value_ptr(row3));
    ImGui::EndDisabled();
}

#pragma once
#include "ecs/EntityRef.h"
#include "serialisation/array.h"

struct SubMesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
};

template <typename>
struct PassComponent
{
    SERIALIZABLE(1, serial::array<SubMesh>) submeshes;
    ecs::EntityID prevMesh = ecs::BadMaxEntity;

    void Serialize(serial::Stream& m)
    {
        m.SerializeComponent(submeshes);
    }

    void Widget()
    {
        if (ImGui::TreeNode("Sub Meshes"))
        {
            int size = submeshes->size();
            if (ImGui::InputInt("Size", &size))
                submeshes->Resize(size < 0 ? 0 : size);

            ImGui::Text("First Index, Index Count");
            for (size_t i = 0; i < submeshes->size(); ++i)
            {
                ImGui::PushID(i);
                SubMesh* submesh = submeshes->data() + i;
                uint32_t data[2] { submesh->firstIndex, submesh->indexCount };
                if (ImGui::InputScalarN(std::to_string(i).c_str(), ImGuiDataType_U32, &data, 2))
                {
                    submesh->firstIndex = data[0];
                    submesh->indexCount = data[1];
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }

    void Destroy()
    {
        submeshes->Destroy();
    }
};

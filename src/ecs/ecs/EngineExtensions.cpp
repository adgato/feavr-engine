#include "EngineExtensions.h"

#include "imgui.h"

namespace ecs
{
    void Widget(Engine& engine, Entity focus)
    {
        if (ImGui::BeginChild("Engine View", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (EntityID e = 0; e < engine.entities.size(); ++e)
            {
                if (!engine.IsValid(e))
                    continue;

                // TreeNode allows for more customization
                std::string entityLabel = "Entity " + std::to_string(e);

                if (e == focus)
                {
                    ImGui::SetNextItemOpen(true);
                    ImGui::SetScrollHereY(0.5f);
                }

                if (ImGui::TreeNode(entityLabel.c_str()))
                {
                    auto [archetype, index] = engine.entities[e];
                    auto& elem = engine.archetypes[archetype];

                    for (const TypeID type : elem.types)
                    {
                        if (ImGui::TreeNode(TypeRegistry::GetInfo(type).name))
                        {
                            TypeRegistry::Widget(type, elem.GetElem(index, type));
                            ImGui::TreePop();
                        }
                    }

                    // Important: Always call TreePop() after TreeNode()
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }
}

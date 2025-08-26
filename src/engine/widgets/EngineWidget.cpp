#include "EngineWidget.h"

#include "imgui.h"
#include <vector>
#include <string>

#include "AutoCompleteWidget.h"
#include "rendering/pass-system/IdentifyPass.h"
#include "rendering/pass-system/Mesh.h"
#include "rendering/pass-system/PassSystem.h"
#include "rendering/pass-system/StencilOutlinePass.h"
#include "rendering/resources/RenderingResources.h"

namespace ecs
{
    std::vector<TypeID> EngineWidget::GetComponentIDs(const Tags& components)
    {
        const size_t size = components.tags->size();

        std::vector<TypeID> componentIDs;
        componentIDs.reserve(size);

        for (size_t i = 0; i < size; ++i)
        {
            const std::string_view componentView(components.tags->data()[i].data());
            for (size_t j = 0; j < typeNames.size(); ++j)
                if (componentView == typeNames[j])
                {
                    componentIDs.emplace_back(j);
                    break;
                }
        }
        std::ranges::sort(componentIDs);

        return componentIDs;
    }

    void EngineWidget::ShowUpdatePopup(Entity e, const TypeID type)
    {
        if (type == BadMaxType)
            return;

        if (popup.mode != UpdatePopupData::DISABED && popup.entity == e && popup.type == type)
        {
            static const char* addText = "add";
            static const char* removeText = "remove";
            ImGui::TextWrapped("Are you sure you want to %s this component?", popup.mode == UpdatePopupData::ADD ? addText : removeText);
            ImGui::TextWrapped("This may result in a crash.");
            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                if (popup.mode == UpdatePopupData::ADD)
                {
                    const auto data = std::unique_ptr<std::byte>(new std::byte[TypeRegistry::GetInfo(popup.type).size]);
                    TypeRegistry::CopyDefault(popup.type, data.get());
                    engine.RawAdd(popup.entity, data.get(), TypeRegistry::GetInfo(popup.type));
                } else if (popup.mode == UpdatePopupData::REMOVE)
                    engine.RawRemove(popup.entity, popup.type);

                popup.mode = UpdatePopupData::DISABED;
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
                popup.mode = UpdatePopupData::DISABED;
        } else if (ImGui::SmallButton("+"))
        {
            popup.mode = UpdatePopupData::ADD;
            popup.entity = e;
            popup.type = type;
        }
    }

    void EngineWidget::RefeshTags()
    {
        tagsSet.clear();
        tagsSet.insert(typeNameSet.begin(), typeNameSet.end());

        for (auto [_, tags] : tagView)
            for (size_t i = 0; i < tags.tags->size(); ++i)
                tagsSet.insert(tags.tags->data()[i].data());

        tags.assign(tagsSet.begin(), tagsSet.end());
    }

    bool EngineWidget::ConsiderEntity(Entity e, const std::vector<TypeID>& includeTypes, const std::vector<TypeID>& excludeTypes)
    {
        if (!engine.IsValid(e))
            return false;

        auto [archetype, index] = engine.entities[e];
        const std::vector<TypeID>& types = engine.archetypes[archetype].types;

        if (!IsEntityRelevant(types, includeTypes, excludeTypes))
            return false;

        if (Tags* tags = engine.TryGet<Tags>(e))
        {
            for (size_t i = 0; i < includeTags.tags->size(); ++i)
            {
                if (!tags->ContainsTag(includeTags.tags->data()[i]))
                    return false;
            }
            for (size_t i = 0; i < excludeTags.tags->size(); ++i)
            {
                if (tags->ContainsTag(excludeTags.tags->data()[i]))
                    return false;
            }
            return true;
        }
        return includeTags.tags->size() == 0;
    }

    EngineWidget::EngineWidget(Engine& engine, rendering::PassSystem& passSys)
        : engine(engine), tagView(engine), outlineView(engine), outlineMat(engine, passSys)
    {
        typeNames.reserve(TypeRegistry::RegisteredCount());
        for (TypeID i = 0; i < TypeRegistry::RegisteredCount(); ++i)
        {
            const char* typeName = TypeRegistry::GetInfo(i).name;
            typeNames.push_back(typeName);
            typeNameSet.insert(typeName);
        }
        RefeshTags();
    }

    void EngineWidget::SetHotEntity(Entity hotEntity, const glm::vec2 coord)
    {
        if (showEntities.size() == 0)
        {
            showEntities.push_back(hotEntity);
            showHot = false;
        } else
        {
            showHot = showEntities[0] == hotEntity;
            showEntities[0] = hotEntity;
        }
        hotViewPos = coord;
        hotChanged = true;
    }

    bool EngineWidget::IsEntityRelevant(const std::vector<TypeID>& types, const std::vector<TypeID>& includeTypes, const std::vector<TypeID>& excludeTypes)
    {
        // similar to EngineView::Without::IsRelevant
        if (types.size() < includeTypes.size())
            return false;

        if (includeTypes.size() > 0)
        {
            size_t j = 0;
            for (size_t i = 0; i < types.size(); ++i)
            {
                if (includeTypes[j] == types[i] && ++j == includeTypes.size())
                    break;
            }
            if (j < includeTypes.size())
                return false;
        }
        if (excludeTypes.size() > 0)
        {
            size_t j = 0;
            for (size_t i = 0; i < types.size(); ++i)
            {
                const TypeID type = types[i];
                while (excludeTypes[j] < type)
                    if (++j >= excludeTypes.size())
                        return true;
                if (excludeTypes[j] == type)
                    return false;
            }
        }
        return true;
    }

    void EngineWidget::Search()
    {
        const std::vector<TypeID> includeTypes = GetComponentIDs(includeComponents);
        const std::vector<TypeID> excludeTypes = GetComponentIDs(excludeComponents);

        std::erase_if(showEntities, [&includeTypes, &excludeTypes, this](Entity e)
        {
            return !ConsiderEntity(e, includeTypes, excludeTypes);
        });

        const std::unordered_set<TypeID> oldShowEntities(showEntities.begin(), showEntities.end());

        for (EntityID e = 0; e < engine.entities.size(); ++e)
        {
            if (!oldShowEntities.contains(e) && ConsiderEntity(e, includeTypes, excludeTypes))
                showEntities.push_back(e);
        }
    }

    void EngineWidget::SearchTree()
    {
        ImGui::PushID("Include");
        ImGui::SeparatorText("Include");
        int includeSelect = -1;
        ImGui::SetNextItemWidth(100);
        if (includeTagsSelector.AutoComplete("##Add", &includeSelect, tags))
        {
            const char* tag = tags[includeSelect].c_str();
            if (typeNameSet.contains(tags[includeSelect]))
                includeComponents.AddTag(tag);
            else
                includeTags.AddTag(tag);
        }

        ImGui::PushID("Components");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.5f, 1));
        includeComponents.Widget();
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("Tags");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.5f, 1));
        includeTags.Widget();
        ImGui::PopStyleColor();
        ImGui::PopID();

        ImGui::PopID();

        ImGui::PushID("Exclude");
        ImGui::SeparatorText("Exclude");
        int excludeSelect = -1;
        ImGui::SetNextItemWidth(100);
        if (excludeTagsSelector.AutoComplete("##Add", &excludeSelect, tags))
        {
            const char* tag = tags[excludeSelect].c_str();
            if (typeNameSet.contains(tags[excludeSelect]))
                excludeComponents.AddTag(tag);
            else
                excludeTags.AddTag(tag);
        }

        ImGui::PushID("Components");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1));
        excludeComponents.Widget();
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("Tags");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.2f, 0.2f, 1));
        excludeTags.Widget();
        ImGui::PopStyleColor();
        ImGui::PopID();

        ImGui::PopID();

        ImGui::Separator();
        if (ImGui::Button("Search!", ImVec2(100, 20)))
            Search();

        ImGui::SameLine();

        if (ImGui::Button("Refresh Tags"))
            RefeshTags();

        ImGui::SameLine();
        if (ImGui::Button("Highlight All"))
        {
            for (auto [submesh, _] : outlineView)
                engine.Remove<PassComponent<StencilOutlinePass>>(submesh);

            std::vector<EntityID> submeshes {};
            for (Entity entity : showEntities)
                outlineMat.Apply(entity, true);
        }
    }

    void EngineWidget::SetWindowSplit()
    {
        bool focusWindow = false;
        if (ImGui::IsKeyPressed(ImGuiKey_V, false))
        {
            split.toggleOn ^= true;
            if (split.toggleOn)
            {
                if (split.prev < 0.05f)
                    split.prev = 0.05f;
                split.target = split.prev;
                focusWindow = true;
            } else
            {
                split.prev = split.target;
                split.target = 0;
            }
        }

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float total_width = viewport->WorkSize.x;
        const float total_height = viewport->WorkSize.y;

        constexpr float animateSpeed = 20;
        const float lerp = glm::exp(-animateSpeed * rendering::deltaTime);
        split.ratio = lerp * split.ratio + (1 - lerp) * split.target;
        float panel_width = total_width * split.ratio;
        if (panel_width < 1)
            panel_width = 1;
        const float game_width = total_width - panel_width;
        const ImVec2 window_pos = ImVec2(game_width, 0);

        // Create splitter area at the left edge of the window
        const ImVec2 splitter_pos = ImVec2(glm::min(window_pos.x, total_width - 8), window_pos.y);
        const ImVec2 splitter_size = ImVec2(8, total_height);

        // Check if mouse is over splitter area
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        const bool is_over_splitter = (mouse_pos.x >= splitter_pos.x &&
                                       mouse_pos.x <= splitter_pos.x + splitter_size.x &&
                                       mouse_pos.y >= splitter_pos.y &&
                                       mouse_pos.y <= splitter_pos.y + splitter_size.y);


        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        if (is_over_splitter)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

            // Draw hover line
            const ImU32 line_color = ImGui::GetColorU32(ImGuiCol_Separator);
            draw_list->AddLine(
                ImVec2(game_width, window_pos.y),
                ImVec2(game_width, window_pos.y + total_height),
                line_color,
                2.0f
            );

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                split.dragging = true;
            }
        }

        if (split.dragging)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

            // Draw active line
            const ImU32 line_color = ImGui::GetColorU32(ImGuiCol_Separator);
            draw_list->AddLine(
                ImVec2(game_width, window_pos.y),
                ImVec2(game_width, window_pos.y + total_height),
                line_color,
                2.0f
            );

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                split.target = (total_width - mouse_pos.x) / total_width;
                split.ratio = split.target;
                split.toggleOn = true;
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                split.dragging = false;
        }

        if (focusWindow)
            ImGui::SetNextWindowFocus();
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panel_width + 1, total_height), ImGuiCond_Always);
    }

    void EngineWidget::EngineTable()
    {
        TypeID hotType = BadMaxType;


        constexpr ImGuiTableFlags flags =
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_ContextMenuInBody |
                ImGuiTableFlags_SizingFixedSame;

        if (ImGui::BeginTable("Engine View", showTypes.size() + 1, flags, ImGui::GetContentRegionAvail()))
        {
            // Header row with component type selectors
            ImGui::TableSetupColumn("##Entity", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            for (size_t k = 0; k < showTypes.size(); ++k)
            {
                ImGui::TableSetupColumn("##Component");
            }

            ImGui::TableSetupScrollFreeze(1, 1);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

            ImGui::TableNextColumn();
            ImGui::Text("Entity");

            ImGui::PushID("ComponentHeaders");
            for (size_t k = 0; k < showTypes.size(); ++k)
            {
                ImGui::PushID(k);
                ImGui::TableNextColumn();

                int t = showTypes[k].type;
                ImGui::SetNextItemWidth(-1);
                if (showTypes[k].widget.AutoComplete("##TypeCombo", &t, typeNames))
                    showTypes[k].type = t;
                ImGui::PopID();
            }
            ImGui::PopID();

            // Entity rows
            ImGui::PushID("EntityRows");
            for (size_t i = 0; i < showEntities.size(); ++i)
            {
                ImGui::PushID(i);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                EntityID entity = showEntities[i];

                if (ImGui::InputScalar("##EntityID", ImGuiDataType_U32, &entity))
                    showEntities[i] = entity;

                if (!engine.IsValid(entity))
                {
                    ImGui::PopID();
                    continue;
                }

                auto [archetype, index] = engine.entities[entity];
                const auto& elem = engine.archetypes[archetype];

                // If tree node is expanded, list all component types
                ImGui::SameLine();
                const bool expandEntity = ImGui::TreeNode("");
                if (expandEntity)
                {
                    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
                    for (const TypeID& typeID : elem.types)
                    {
                        ImGui::Bullet();
                        if (ImGui::Selectable(TypeRegistry::GetInfo(typeID).name))
                            hotType = typeID;
                        else if (ImGui::IsItemHovered() && ImGui::IsKeyReleased(ImGuiKey_MouseMiddle))
                        {
                            popup.mode = UpdatePopupData::REMOVE;
                            popup.entity = entity;
                            popup.type = typeID;
                            hotType = typeID;
                        }
                    }
                    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
                    ImGui::TreePop();
                }

                // Component columns for this entity
                for (size_t k = 0; k < showTypes.size(); ++k)
                {
                    ImGui::TableNextColumn();
                    ImGui::PushID(k);

                    const TypeID showType = showTypes[k].type;
                    if (expandEntity)
                    {
                        const bool removing = popup.mode == UpdatePopupData::REMOVE && popup.entity == entity && popup.type == showType;
                        if (elem.StoresType(showType) && !removing)
                            TypeRegistry::Widget(showType, elem.GetElem(index, showType));
                        else
                            ShowUpdatePopup(entity, showType);
                    }

                    ImGui::PopID();
                }
                ImGui::PopID();
            }
            ImGui::PopID();
            ImGui::EndTable();
        }

        if (hotType != BadMaxType)
        {
            if (showTypes.size() == 0)
                showTypes.resize(1);
            showTypes[0].type = hotType;
        }
    }

    void EngineWidget::Windows()
    {
        SetWindowSplit();
        if (ImGui::Begin("Engine View", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
        {
            if (ImGui::TreeNode("Search ECS"))
            {
                SearchTree();
                ImGui::TreePop();
            }

            int entityCount = showEntities.size();
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputInt("Entities", &entityCount))
            {
                if (entityCount < 0)
                    entityCount = 0;
                showEntities.resize(entityCount, BadMaxEntity);
            }

            int typesCount = showTypes.size();
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputInt("Components", &typesCount))
            {
                if (typesCount < 0)
                    typesCount = 0;

                const size_t oldSize = showTypes.size();
                showTypes.resize(typesCount);
                for (size_t i = oldSize; i < typesCount; ++i)
                    showTypes[i].type = BadMaxType;
            }

            EngineTable();
        }
        ImGui::End();

        if (hotChanged)
        {
            ImGui::SetNextWindowPos(ImVec2 { hotViewPos.x, hotViewPos.y });
            hotChanged = false;
        }

        const bool showHotView = showHot && showEntities.size() > 0;
        if (showHotView && ImGui::Begin("Hot View", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
        {
            if (ImGui::SmallButton("X"))
                showHot = false;
            if (!showHot || ImGui::IsItemHovered() && ImGui::IsKeyReleased(ImGuiKey_MouseMiddle))
            {
                for (auto [submesh, _] : outlineView)
                    engine.Remove<PassComponent<StencilOutlinePass>>(submesh);
            }

            Entity e = showEntities[0];
            if (engine.IsValid(e))
            {
                TypeID hotType = BadMaxType;
                auto [archetype, index] = engine.entities[e];
                const auto& elem = engine.archetypes[archetype];

                ImGui::SameLine();
                if (ImGui::TreeNode("Entity", "Entity %d", e))
                {
                    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
                    for (const TypeID& typeID : elem.types)
                    {
                        ImGui::Bullet();
                        if (ImGui::Selectable(TypeRegistry::GetInfo(typeID).name))
                            hotType = typeID;
                    }
                    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
                    ImGui::TreePop();
                }
                ImGui::Separator();
                if (showTypes.size() > 0)
                {
                    const TypeID type = showTypes[0].type;
                    if (elem.StoresType(type))
                        TypeRegistry::Widget(type, elem.GetElem(index, type));
                }
                if (hotType != BadMaxType)
                {
                    if (showTypes.size() == 0)
                        showTypes.resize(1);
                    showTypes[0].type = hotType;
                }
            }
        }
        if (showHotView)
            ImGui::End();
    }
}

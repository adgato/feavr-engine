#include "AutoCompleteWidget.h"

#include <cstring>

#include "imgui.h"

namespace widgets
{
    void AutoCompleteWidget::ClearBuffer()
    {
        for (size_t i = 0; i < 256; ++i)
            buffer[i] = '\0';
    }

    bool AutoCompleteWidget::AutoComplete(const char* label, int* index, const std::span<std::string>& items)
    {
        buffer[255] = '\0';
        bool modified = false;
        if (ImGui::BeginCombo(label, *index >= 0 && *index < items.size() ? items[*index].c_str() : "Choose..."))
        {
            // initially focus text box
            if (!alreadyOpen)
                ImGui::SetKeyboardFocusHere();

            if (ImGui::InputText("##search", buffer.get(), 256) || !alreadyOpen)
            {
                selectedIndex = 0;

                filteredIndices.clear();
                for (int i = 0; i < items.size(); i++)
                {
                    if (std::strlen(buffer.get()) == 0 || items[i].starts_with(buffer.get()))
                        filteredIndices.push_back(i);
                }
            }

            bool movedArrow = false;

            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && selectedIndex < filteredIndices.size() - 1)
            {
                movedArrow = true;
                selectedIndex++;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && selectedIndex > 0)
            {
                movedArrow = true;
                selectedIndex--;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && selectedIndex >= 0 && selectedIndex < filteredIndices.size())
            {
                *index = filteredIndices[selectedIndex];
                ClearBuffer();
                ImGui::CloseCurrentPopup();
                modified = true;
            }

            // Draw filtered items
            for (int i = 0; i < filteredIndices.size(); i++)
            {
                const char* option = items[filteredIndices[i]].c_str();
                const bool isSelected = i == selectedIndex;

                if (ImGui::Selectable(option, isSelected))
                {
                    *index = filteredIndices[i];
                    ClearBuffer();
                    ImGui::CloseCurrentPopup();
                    modified = true;
                }

                if (movedArrow && isSelected)
                    ImGui::SetScrollHereY();
            }

            ImGui::EndCombo();
            alreadyOpen = true;
        } else
        {
            alreadyOpen = false;
            selectedIndex = 0;
        }
        return modified;
    }
}

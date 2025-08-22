#include "Tags.h"

#include "imgui.h"

void Tags::Serialize(serial::Stream& m)
{
    m.SerializeComponent(tags);
}

void Tags::AddTag(const char* text)
{
    const size_t len = std::strlen(text);
    tags->push_back(String::NewFromData(text, len + 1));
}

bool Tags::ContainsTag(const String& tag)
{
    for (size_t i = 0; i < tags->size(); ++i)
    {
        const String& compare = tags->data()[i];
        if (tag.size() != compare.size())
            goto nomatch;

        for (size_t j = 0; j < tag.size(); ++j)
        {
            if (tag.data()[j] != compare.data()[j])
                goto nomatch;
        }
        return true;
    nomatch:;
    }
    return false;
}

void Tags::RemoveTag(const size_t index)
{
    tags->data()[index].Destroy();
    for (size_t i = index + 1; i < tags->size(); ++i)
        tags->data()[i - 1] = tags->data()[i];
    tags->Resize(tags->size() - 1);
}

void Tags::Widget()
{
    char buffer[256];
    int remove = -1;

    float currentLineWidth = 0;
    const float totalAvailableWidth = ImGui::GetContentRegionAvail().x;

    for (size_t i = 0; i < tags->size(); ++i)
    {
        ImGui::PushID(i);

        std::strncpy(buffer, tags->data()[i].data(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        const float inputWidth = ImGui::CalcTextSize(buffer).x + 20;
        const float thisItemWidth = inputWidth + ImGui::GetStyle().ItemSpacing.x;

        // Check if this item fits on current line
        if (i > 0 && currentLineWidth + thisItemWidth > totalAvailableWidth)
            currentLineWidth = thisItemWidth;
        else
        {
            if (i > 0)
                ImGui::SameLine();
            currentLineWidth += thisItemWidth;
        }

        ImGui::SetNextItemWidth(inputWidth);
        if (ImGui::InputText("##text", buffer, sizeof(buffer)))
        {
            buffer[sizeof(buffer) - 1] = '\0';
            const size_t len = std::strlen(buffer);
            tags->data()[i].Resize(len + 1);
            std::strncpy(tags->data()[i].data(), buffer, len + 1);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
            remove = i;

        ImGui::PopID();
    }

    if (remove >= 0)
        RemoveTag(remove);

    const float inputWidth = ImGui::CalcTextSize("+").x + 10;
    const float thisItemWidth = inputWidth + ImGui::GetStyle().ItemSpacing.x;

    if (tags->size() > 0 && currentLineWidth + thisItemWidth <= totalAvailableWidth)
        ImGui::SameLine();

    if (ImGui::Button("+"))
        AddTag("New_Tag");
}

void Tags::Destroy()
{
    tags->Destroy();
}

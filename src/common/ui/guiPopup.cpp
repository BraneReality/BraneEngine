//
// Created by wirewhiz on 21/07/22.
//

#include "guiPopup.h"
#include "imgui_internal.h"

GUIPopup::GUIPopup(const std::string& name) : _name(name)
{
    _id = ImGui::GetID(name.c_str());
    ImGui::OpenPopup(name.c_str());
}

void GUIPopup::draw()
{
    if(ImGui::BeginPopupEx(_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration))
    {
        drawBody();
        ImGui::EndPopup();
    }
}

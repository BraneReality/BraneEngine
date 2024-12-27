//
// Created by eli on 5/21/2022.
//

#include "consoleWindow.h"
#include "imgui_stdlib.h"
#include <runtime/logging.h>
#include <ui/gui.h>

ConsoleWindow::ConsoleWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _name = "Console";
    _listenerIndex = Logging::addListener([this](const auto& log) {
        CachedLog cl{log.toString(), log.level, 1};
        for(auto c : cl.text)
            if(c == '\n')
                ++cl.lineCount;
        _messages.push_back(cl);
    });
}

void ConsoleWindow::displayContent()
{
    ImGui::Checkbox("Auto Scroll", &_autoScroll);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0, 0, 0, 1});
    ImGui::BeginChild("messages");
    const ImVec4 textColors[] = {{1, 0, 0, 1}, {1, 1, 0, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
    ImGui::PushTextWrapPos();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, {0, 0, 0, 1});
    ImGui::PushFont(_ui.fonts()[2]);
    int messageID = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 1});
    for(auto& m : _messages)
    {
        ImGui::PushID(messageID++);
        ImVec2 size = {ImGui::GetContentRegionAvail().x,
                       (ImGui::GetStyle().FramePadding.y * 2 + ImGui::GetFontSize()) * m.lineCount};
        if(ImGui::IsRectVisible(size))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, textColors[static_cast<size_t>(m.level)]);
            ImGui::InputTextMultiline("##m", &m.text, size, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
        }
        else
            ImGui::Dummy(size);
        ImGui::PopID();
    }
    if(_autoScroll)
        ImGui::SetScrollHereY(1.0f);
    ImGui::PopStyleVar();
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

ConsoleWindow::~ConsoleWindow() { Logging::removeListener(_listenerIndex); }

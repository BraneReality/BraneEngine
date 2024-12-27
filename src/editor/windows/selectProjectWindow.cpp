//
// Created by eli on 8/16/2022.
//

#include "selectProjectWindow.h"
#include <imgui.h>
#include "editor/editor.h"
#include "fileManager/fileManager.h"
#include "imgui_stdlib.h"
#include "tinyfiledialogs.h"
#include "ui/gui.h"

SelectProjectWindow::SelectProjectWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _projectPath = std::filesystem::current_path().append("/Brane Projects/").string();
    loadRecent();
    _name = "Select Project";
    // TODO store a default project folder in config or grab the documents folder from the OS
    _flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;
}

void SelectProjectWindow::displayContent()
{
    ImVec2 windowSize = {600, 350};
    ImGui::SetWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetWindowPos(
        {(ImGui::GetIO().DisplaySize.x - windowSize.x) / 2, (ImGui::GetIO().DisplaySize.y - windowSize.y) / 2},
        ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.3f);
    ImGui::PushFont(_ui.fonts()[1]);
    ImGui::Text("Create New");
    ImGui::PopFont();
    ImGui::InputText("Name", &_projectName, ImGuiInputTextFlags_AutoSelectAll);
    if(ImGui::Button("Select##Directory"))
    {
        const char* pathCStr = tinyfd_selectFolderDialog("Select Directory", _projectPath.c_str());
        if(pathCStr)
            _projectPath = pathCStr;
    }
    ImGui::SameLine();
    ImGui::InputText("Directory", &_projectPath, ImGuiInputTextFlags_AutoSelectAll);

    if(ImGui::Button("Create"))
    {
        if(!std::filesystem::exists(std::filesystem::path{_projectPath} / _projectName / (_projectName + ".brane")))
        {
            _recentProjects.insert(
                _recentProjects.begin(),
                {_projectName,
                 (std::filesystem::path{_projectPath} / _projectName / (_projectName + ".brane")).string()});
            _selectedProject = 0;
            saveRecents();
            Runtime::getModule<Editor>()->createProject(_projectName, _projectPath);
        }
        else
            _creationStatus = "Project already exists!";
    }
    if(!_creationStatus.empty())
        ImGui::Text("%s", _creationStatus.c_str());
    ImGui::Separator();

    ImGui::PushFont(_ui.fonts()[1]);
    ImGui::Text("Recent Projects");
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.1f, 0.1f, 0.1f, 1.0f});
    ImGui::BeginChild("Recent", {ImGui::GetContentRegionMax().x, ImGui::GetWindowHeight() * 0.45f}, 0);
    for(uint8_t i = 0; i < _recentProjects.size(); ++i)
    {
        auto& p = _recentProjects[i];
        if(ImGui::Selectable(p.name.c_str(), _selectedProject == i))
        {
            _selectedProject = i;
        }
    }
    if(ImGui::IsMouseDoubleClicked(0) && _selectedProject >= 0)
    {
        saveRecents();
        Runtime::getModule<Editor>()->loadProject(_recentProjects[_selectedProject].path);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::BeginDisabled(_selectedProject < 0);
    if(ImGui::Button("Select##Project"))
    {
        saveRecents();
        Runtime::getModule<Editor>()->loadProject(_recentProjects[_selectedProject].path);
    }
    ImGui::EndDisabled();
    if(ImGui::Button("Open From File"))
    {
        const char* filters[] = {"*.brane"};
        std::string path = tinyfd_openFileDialog("Select Brane File", nullptr, 1, filters, "Brane Project Files", 0);
        _recentProjects.insert(_recentProjects.begin(), {"no name (This is a bug)", path});
        _selectedProject = 0;
        saveRecents();
        Runtime::getModule<Editor>()->loadProject(_recentProjects[_selectedProject].path);
    }
    ImGui::PopStyleVar();
}

void SelectProjectWindow::loadRecent()
{
    try
    {
        Json::Value projects;
        if(FileManager::readFile("cache/recentProjects.json", projects))
        {
            if(projects.isMember("lastPath"))
                _projectPath = projects["lastPath"].asString();
            for(auto& proj : projects["projects"])
            {
                if(!std::filesystem::exists(proj["path"].asString()))
                    continue;
                RecentProject p;
                p.name = proj["name"].asString();
                if(p.name.empty())
                    p.name = "[No name found]";
                p.path = proj["path"].asString();

                _recentProjects.push_back(p);
            }
            return;
        }
    }
    catch(...)
    {
        Runtime::log("no usable project cache found, a new one will be created");
    }
}

void SelectProjectWindow::saveRecents()
{
    assert(_selectedProject >= 0);
    if(_selectedProject != 0)
    {
        auto project = _recentProjects[_selectedProject];
        _recentProjects.erase(_recentProjects.begin() + _selectedProject);
        _recentProjects.insert(_recentProjects.begin(), project);
    }
    Json::Value projects;
    for(auto& p : _recentProjects)
    {
        Json::Value proj;
        proj["name"] = p.name;
        proj["path"] = p.path;
        projects["projects"].append(proj);
    }
    projects["lastPath"] = _projectPath;
    FileManager::writeFile("cache/recentProjects.json", projects);
}

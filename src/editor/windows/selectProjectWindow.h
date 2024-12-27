//
// Created by eli on 8/16/2022.
//

#ifndef BRANEENGINE_SELECTPROJECTWINDOW_H
#define BRANEENGINE_SELECTPROJECTWINDOW_H

#include <filesystem>
#include <string>
#include <vector>
#include "editorWindow.h"

class SelectProjectWindow : public EditorWindow
{
    struct RecentProject
    {
        std::string name;
        std::string path;
    };

    std::string _projectName = "new project";
    std::string _projectPath;
    std::string _creationStatus;

    std::vector<RecentProject> _recentProjects;
    int _selectedProject = -1;

    void loadRecent();

    void saveRecents();

    void displayContent() override;

  public:
    SelectProjectWindow(GUI& ui, Editor& editor);
};

#endif // BRANEENGINE_SELECTPROJECTWINDOW_H

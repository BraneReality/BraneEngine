//
// Created by wirewhiz on 20/07/22.
//

#ifndef BRANEENGINE_ASSETBROWSERWIDGET_H
#define BRANEENGINE_ASSETBROWSERWIDGET_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "fileManager/fileManager.h"
#include "glm/vec2.hpp"
#include "ui/gui.h"
#include "ui/guiPopup.h"
#include "utility/asyncQueue.h"

class AssetBrowserWidget
{
  private:
    std::filesystem::path _rootPath;
    std::unique_ptr<FileManager::Directory> _root;
    FileManager::Directory* _currentDir;
    glm::ivec2 _selectedFiles = {-1, -1};
    size_t _firstSelected = 0;

    std::vector<std::filesystem::directory_entry> _contents;

    GUI& _ui;
    bool _allowEdits;
    AsyncQueue<std::function<void()>> _mainThreadActions;

    enum class FileType
    {
        unknown,
        directory,
        normal,
        source,
        asset
    };

    const char* getIcon(const std::filesystem::path& path);

    FileType getFileType(const std::filesystem::directory_entry& file);

    void displayDirectoriesRecursive(FileManager::Directory* dir);

  public:
    AssetBrowserWidget(GUI& ui, bool allowEdits);

    void displayDirectoryTree();

    void displayFiles();

    void displayFullBrowser();

    std::filesystem::path currentDirectory();

    void reloadCurrentDirectory();

    void setDirectory(FileManager::Directory* dir);
};

#endif // BRANEENGINE_ASSETBROWSERWIDGET_H

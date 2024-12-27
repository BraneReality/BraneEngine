//
// Created by eli on 6/28/2022.
//

#ifndef BRANEENGINE_EDITOR_H
#define BRANEENGINE_EDITOR_H

#include <memory>
#include "assets/assetCache.h"
#include "assets/assetID.h"
#include "braneProject.h"
#include "graphics/shaderCompiler.h"
#include "utility/jsonVersioner.h"
#include <runtime/module.h>
#include <unordered_map>

namespace net
{
    class Connection;
}

class GUI;

class GUIWindow;

class EditorAsset;

class Editor : public Module
{
    GUI* _ui;
    GUIWindow* _selectProjectWindow = nullptr;

    JsonVersionTracker _jsonTracker;
    AssetCache _cache;
    BraneProject _project;
    ShaderCompiler _shaderCompiler;

    void addMainWindows();

    void drawMenu();

  public:
    Editor();

    void start() override;

    void loadProject(const std::filesystem::path& filepath);

    void createProject(const std::string& name, const std::filesystem::path& directory);

    void reloadAsset(std::shared_ptr<EditorAsset> asset);

    BraneProject& project();

    JsonVersionTracker& jsonTracker();

    AssetCache& cache();

    ShaderCompiler& shaderCompiler();

    static const char* name();
};

#endif // BRANEENGINE_EDITOR_H

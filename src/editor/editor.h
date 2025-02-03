//
// Created by eli on 6/28/2022.
//

#ifndef BRANEENGINE_EDITOR_H
#define BRANEENGINE_EDITOR_H

#include <memory>
#include "assets/assetCache.h"
#include "assets/assetID.h"
#include "braneProject.h"
#include "editor/state/versioning.h"
#include "graphics/shaderCompiler.h"
#include "state/trackedObject.h"
#include <runtime/module.h>
#include <unordered_map>

namespace net
{
    class Connection;
}

class GUI;

class GUIWindow;

class EditorAsset;

struct EditorState : public TrackedObject
{
    Shared<TrackedValue<Option<Shared<EditorAsset>>>> focusedAsset;
    EditorState();

    void initMembers(Option<std::shared_ptr<TrackedType>> parent) override;
};

class Editor : public Module
{
    GUI* _ui;
    GUIWindow* _selectProjectWindow = nullptr;

    AssetCache _cache;
    Option<BraneProject> _project;
    ShaderCompiler _shaderCompiler;
    EditorActionManager _actionManager;

    Shared<EditorState> _state;

    void addMainWindows();

    void drawMenu();

  public:
    Editor();

    void start() override;

    void loadProject(const std::filesystem::path& filepath);

    void createProject(const std::string& name, const std::filesystem::path& directory);

    void reloadAsset(Shared<EditorAsset> asset);

    Option<BraneProject*> project();

    Shared<EditorState> state() const;

    EditorActionManager& actionManager();

    AssetCache& cache();

    ShaderCompiler& shaderCompiler();

    static const char* name();
};

#endif // BRANEENGINE_EDITOR_H

//
// Created by eli on 6/28/2022.
//

#include "editor.h"

#include "ui/gui.h"
#include "windows/assetBrowserWindow.h"
#include "windows/consoleWindow.h"
#include "windows/dataWindow.h"
#include "windows/entitiesWindow.h"
#include "windows/memoryManagerWindow.h"
#include "windows/renderWindow.h"
#include "windows/selectProjectWindow.h"
#include "windows/syncWindow.h"

#include "assets/assetManager.h"
#include "assets/editorAsset.h"
#include "assets/types/materialAsset.h"
#include "assets/types/shaderAsset.h"
#include "editorEvents.h"
#include "fileManager/fileManager.h"
#include "fileManager/fileWatcher.h"
#include "graphics/graphics.h"
#include "graphics/material.h"
#include "networking/networking.h"

#include "tinyfiledialogs.h"

// The editor specific fetch asset function
class EditorAssetLoader : public AssetLoader
{

    AsyncData<Asset*> loadAsset(const AssetID& inId, bool incremental) override
    {
        assert(!inId.empty());
        AsyncData<Asset*> asset;
        auto castId = inId.as<BraneAssetID>();
        if(!castId)
        {
            asset.setError("Can only fetch Brane Asset IDs");
            return asset;
        }
        auto id = castId.value();

        Editor* editor = Runtime::getModule<Editor>();

        if(editor->cache().hasAsset(id))
        {
            ThreadPool::enqueue([this, editor, asset, id]() {
                Asset* cachedAsset = editor->cache().getAsset(id);
                asset.setData(cachedAsset);
            });
            return asset;
        }

        std::shared_ptr<EditorAsset> editorAsset = editor->project().getEditorAsset(id);
        if(editorAsset)
        {
            ThreadPool::enqueue([this, editorAsset, editor, asset, id]() {
                Asset* a = editorAsset->buildAsset(id);
                if(!a)
                {
                    asset.setError("Could not build " + id.toString() + " from " + editorAsset->name());
                    return;
                }
                editor->cache().cacheAsset(a);
                asset.setData(a);
            });
            return asset;
        }

        if(id.domain.empty())
        {
            asset.setError("Asset with id " + std::string(id.toString()) +
                           " was not found and can not be remotely fetched since it lacks a server address");
            return asset;
        }
        auto* nm = Runtime::getModule<NetworkManager>();
        if(incremental)
        {
            nm->async_requestAssetIncremental(id).then([this, asset](Asset* ptr) {
                asset.setData(ptr);
            }).onError([asset](std::string error) { asset.setError(error); });
        }
        else
        {
            nm->async_requestAsset(id).then([this, asset](Asset* ptr) {
                asset.setData(ptr);
            }).onError([asset](std::string error) { asset.setError(error); });
        }
        return asset;
    }
};

void Editor::start()
{
    Runtime::getModule<AssetManager>()->addLoader(std::make_unique<EditorAssetLoader>());
    _ui = Runtime::getModule<GUI>();
    _selectProjectWindow = _ui->addWindow<SelectProjectWindow>(*this);

    _ui->setMainMenuCallback([this]() { drawMenu(); });
    _ui->addEventListener<GUIEvent>("projectLoaded", nullptr, [this](auto evt) {
        if(_selectProjectWindow)
        {
            _selectProjectWindow = nullptr;
            _ui->clearWindows();
            addMainWindows();
        }
    });
    Runtime::getModule<graphics::VulkanRuntime>()->onWindowClosed([this]() {
        if(!_project.loaded())
            return true;
        if(!_project.unsavedChanges())
            return true;
        int input = tinyfd_messageBox(nullptr, "Unsaved changes! Do you want to save?", "yesnocancel", "warning", 1);
        if(input == 1)
            _project.save();
        else
            Runtime::log("User chose not to save on exit");
        return input != 0;
    });
}

const char* Editor::name()
{
    return "editor";
}

void Editor::addMainWindows()
{
    auto* dataWindow = _ui->addWindow<DataWindow>(*this);
    auto* assetBrowser = _ui->addWindow<AssetBrowserWindow>(*this);
    auto* console = _ui->addWindow<ConsoleWindow>(*this);
    auto* entities = _ui->addWindow<EntitiesWindow>(*this);
    auto* renderer = _ui->addWindow<RenderWindow>(*this);

    ImGuiID root = ImGui::GetID("DockingRoot");
    ImGui::DockBuilderRemoveNode(root);
    root = ImGui::DockBuilderAddNode(
        root, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton);
    ImGui::DockBuilderSetNodeSize(root, ImGui::GetCurrentContext()->CurrentViewport->WorkSize);

    ImGuiID assetDataNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Right, .2f, nullptr, &root);
    ImGuiID consoleWindowNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Down, .2f, nullptr, &root);
    ImGuiID entitiesWindowNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Left, .2f, nullptr, &root);

    ImGui::DockBuilderDockWindow(dataWindow->name().c_str(), assetDataNode);
    ImGui::DockBuilderDockWindow(console->name().c_str(), consoleWindowNode);
    ImGui::DockBuilderDockWindow(assetBrowser->name().c_str(), consoleWindowNode);
    ImGui::DockBuilderDockWindow(entities->name().c_str(), entitiesWindowNode);
    ImGui::DockBuilderDockWindow(renderer->name().c_str(), root);

    // Attempt at recreating the (for some reason commented out) ImGuiDockNodeFlags_NoCentralNode
    ImGui::DockBuilderGetNode(root)->MergedFlags ^= ImGuiDockNodeFlags_CentralNode;
    ImGui::DockBuilderGetNode(root)->UpdateMergedFlags();

    ImGui::DockBuilderFinish(root);

    Runtime::log("Main layout loaded");
}

void Editor::drawMenu()
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            ImGui::Selectable("Create Asset");
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Window"))
        {
            if(ImGui::Selectable("Entities"))
                _ui->addWindow<EntitiesWindow>(*this)->resizeDefault();
            if(ImGui::Selectable("Asset Browser"))
                _ui->addWindow<AssetBrowserWindow>(*this)->resizeDefault();
            if(ImGui::Selectable("Sync Window"))
                _ui->addWindow<SyncWindow>(*this)->resizeDefault();
            if(ImGui::Selectable("Render Preview"))
                _ui->addWindow<RenderWindow>(*this)->resizeDefault();
            if(ImGui::Selectable("Console"))
                _ui->addWindow<ConsoleWindow>(*this)->resizeDefault();
            if(ImGui::Selectable("Data Inspector"))
                _ui->addWindow<DataWindow>(*this)->resizeDefault();
            if(ImGui::Selectable("Memory Manager"))
                _ui->addWindow<MemoryManagerWindow>(*this)->resizeDefault();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(ImGui::IsKeyDown(ImGuiKey_ModCtrl))
    {
        if(ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            if(!ImGui::IsKeyDown(ImGuiKey_ModShift))
                _jsonTracker.undo();
            else
                _jsonTracker.redo();
        }
        else if(ImGui::IsKeyPressed(ImGuiKey_Y))
            _jsonTracker.redo();
        else if(ImGui::IsKeyPressed(ImGuiKey_S))
            _project.save();
    }
}

BraneProject& Editor::project()
{
    return _project;
}

void Editor::loadProject(const std::filesystem::path& filepath)
{
    _project.load(filepath);
    Runtime::getModule<graphics::VulkanRuntime>()->window()->onRefocus([this]() {
        if(_project.fileWatcher())
            _project.fileWatcher()->scanForChanges();
    });
    _ui->sendEvent(std::make_unique<GUIEvent>("projectLoaded"));
}

void Editor::createProject(const std::string& name, const std::filesystem::path& directory)
{
    _project.create(name, directory);
    Runtime::getModule<graphics::VulkanRuntime>()->window()->onRefocus([this]() {
        if(_project.fileWatcher())
            _project.fileWatcher()->scanForChanges();
    });
    _ui->sendEvent(std::make_unique<GUIEvent>("projectLoaded"));
}

JsonVersionTracker& Editor::jsonTracker()
{
    return _jsonTracker;
}

Editor::Editor() : _project(*this)
{
    _cache.setProject(&_project);
}

AssetCache& Editor::cache()
{
    return _cache;
}

ShaderCompiler& Editor::shaderCompiler()
{
    return _shaderCompiler;
}

void Editor::reloadAsset(std::shared_ptr<EditorAsset> asset)
{
    AssetID id = AssetID::parse(asset->data()["id"].asString()).ok();
    _cache.deleteCachedAsset(id);
    auto* am = Runtime::getModule<AssetManager>();
    if(!am->hasAsset(id))
        return;
    Asset* newAsset = asset->buildAsset(id);
    if(!newAsset)
    {
        Runtime::error("Could not reload asset " + id.toString() + "!");
        return;
    }
    am->reloadAsset(newAsset);
    delete newAsset;

    switch(asset->type().type())
    {
        case AssetType::material:
        case AssetType::shader:
            am->fetchAsset<Asset>(id).then([&am](Asset* asset) {
                // Manually fetch dependencies, since it skips that step if an asset is already fully loaded
                am->fetchDependencies(asset, [asset](bool success) {
                    if(success)
                        Runtime::getModule<graphics::VulkanRuntime>()->reloadAsset(asset);
                    else
                        Runtime::warn("Could not reload " + asset->name);
                });
            });
            break;
    }
}

#include "assetBrowserWidget.h"
#include "assets/assetManager.h"
#include "ecs/entity.h"
// #include "editor/assets/types/editorChunkAsset.h"
// #include "editor/assets/types/editorMaterialAsset.h"
// #include "editor/assets/types/editorScriptAsset.h"
// #include "editor/assets/types/editorShaderAsset.h"
#include "editor/editor.h"
#include "editor/editorEvents.h"
#include "fileManager/fileManager.h"
#include "graphics/graphics.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "ui/IconsFontAwesome6.h"
#include "utility/strCaseCompare.h"

class CreateDirectoryPopup : public GUIPopup
{
    AssetBrowserWidget& _widget;
    std::string _dirName = "new dir";

    void drawBody() override
    {
        ImGui::Text("Create Directory:");
        bool enter = ImGui::InputText(
            "##name", &_dirName, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
        if(enter || ImGui::Button("create"))
        {
            ImGui::CloseCurrentPopup();
            FileManager::createDirectory(_widget.currentDirectory() / _dirName);
            _widget.reloadCurrentDirectory();
        }
    }

  public:
    CreateDirectoryPopup(AssetBrowserWidget& widget) : _widget(widget), GUIPopup("Create Directory") {}
};

class CreateAssetPopup : public GUIPopup
{
    AssetBrowserWidget& _widget;
    AssetType _type;
    std::string _assetName;

    ShaderType _shaderType = ShaderType::fragment;

    void drawBody() override
    {
        ImGui::Text("New %s:", _type.toString().c_str());
        bool enter = ImGui::InputText(
            "##name", &_assetName, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
        if(_type == AssetType::shader)
        {
            const char* shaderTypes[] = {"null", "vertex", "fragment"};
            if(ImGui::BeginCombo("Shader Type", shaderTypes[(uint8_t)_shaderType]))
            {
                if(ImGui::Selectable("fragment"))
                    _shaderType = ShaderType::fragment;
                if(ImGui::Selectable("vertex"))
                    _shaderType = ShaderType::vertex;
                ImGui::EndCombo();
            }
        }
        if(enter || ImGui::Button("create"))
        {
            ImGui::CloseCurrentPopup();

            /*
            EditorAsset* asset = nullptr;
            Editor* editor = Runtime::getModule<Editor>();
            switch(_type.type())
            {
                case AssetType::chunk:
                    asset =
                        new EditorChunkAsset(_widget.currentDirectory() / (_assetName + ".chunk"), editor->project());
                    break;
                case AssetType::script:
                {
                    auto scriptAsset =
                        new EditorScriptAsset(_widget.currentDirectory() / (_assetName + ".script"), editor->project());
                    scriptAsset->createDefaultSource();
                    asset = scriptAsset;
                }
                break;
                case AssetType::material:
                    asset = new EditorMaterialAsset(_widget.currentDirectory() / (_assetName + ".material"),
                                                    editor->project());
                    break;
                case AssetType::shader:
                    auto* shader =
                        new EditorShaderAsset(_widget.currentDirectory() / (_assetName + ".shader"), editor->project());
                    shader->createDefaultSource(_shaderType);
                    asset = shader;
                    break;
            }
            if(!asset)
                return;
            asset->save();
            editor->project().registerAssetLocation(asset);
            delete asset;

            */
            Runtime::warn("This button is unimplemented due to refactors, yes, report this");
            _widget.reloadCurrentDirectory();
        }
    }

  public:
    CreateAssetPopup(AssetBrowserWidget& widget, AssetType type) : _widget(widget), GUIPopup("Create Asset")
    {
        _type = type;
        _assetName = "New " + _type.toString();
    }
};

AssetBrowserWidget::AssetBrowserWidget(GUI& ui, bool allowEdits) : _ui(ui), _allowEdits(allowEdits)
{
    _rootPath = Runtime::getModule<Editor>()->project().value()->root() / "assets";
    _root = FileManager::getDirectoryTree(_rootPath);
    setDirectory(_root.get());
}

void AssetBrowserWidget::displayDirectoryTree()
{
    displayDirectoriesRecursive(_root.get());
}

void AssetBrowserWidget::displayDirectoriesRecursive(FileManager::Directory* dir)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    if(dir->children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    ImGui::SetNextItemOpen(dir->open, ImGuiCond_Always);
    bool nodeOpen = ImGui::TreeNodeEx(
        (((dir->open && !dir->children.empty()) ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER) + dir->name).c_str(), flags);
    dir->open = nodeOpen;

    if(ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        setDirectory(dir);
    }
    if(_allowEdits)
    {
        if(dir != _root.get() && ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("directory", &dir, sizeof(FileManager::Directory*));
            ImGui::Text("%s", dir->name.c_str());
            ImGui::EndDragDropSource();
        }

        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* p = ImGui::AcceptDragDropPayload("directory"))
            {
                // TODO notify project file of moved assets
                FileManager::moveFile(_rootPath / (*(FileManager::Directory**)p->Data)->path(),
                                      _rootPath / dir->path());
            }
        }
    }
    if(nodeOpen)
    {

        for(auto& dirChild : dir->children)
        {
            displayDirectoriesRecursive(dirChild.get());
        }
        ImGui::TreePop();
    }
}

void AssetBrowserWidget::setDirectory(FileManager::Directory* dir)
{
    _currentDir = dir;
    _currentDir->setParentsOpen();
    FileManager::refreshDirectoryTree(dir, _rootPath);
    _contents = FileManager::getDirectoryContents(_rootPath / _currentDir->path());
    _selectedFiles = {-1, -1};
}

void AssetBrowserWidget::displayFiles()
{
    bool fileHovered = false;

    if(!_currentDir)
        ImGui::Text("No directory selected");
    else
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.6f});

        // Can't change directory in the middle of displaying it, so cache it in this var instead
        FileManager::Directory* newDirectory = nullptr;
        for(size_t i = 0; i < _contents.size(); ++i)
        {
            auto& file = _contents[i];
            bool isSelected = _selectedFiles.x <= i && i <= _selectedFiles.y;
            FileType type = getFileType(file);

            ImGui::PushID(file.path().c_str());
            if(ImGui::Selectable("##",
                                 isSelected,
                                 ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_AllowDoubleClick,
                                 {0, 0}))
            {
                if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if(type == FileType::asset && _allowEdits)
                    {
                        // Make sure to construct the focus asset event on the main thread
                        auto editor = Runtime::getModule<Editor>();
                        auto asset = editor->project().value()->getEditorAsset(currentDirectory() / file);
                        if(asset)
                            _ui.sendEvent(std::make_unique<FocusAssetEvent>(asset.value()));
                        else
                            Runtime::warn("Could not find clicked asset!");
                    }
                    if(type == FileType::directory)
                    {
                        // Since directories are sorted to the top of files and in the same manner as
                        // directory children, they have the same index
                        newDirectory = _currentDir->children[i].get();
                    }
                }
                if(!ImGui::IsKeyDown(ImGuiKey_ModShift) || _selectedFiles.x == -1)
                {
                    _selectedFiles = {i, i};
                    _firstSelected = i;
                }
                else
                    _selectedFiles = {std::min(_firstSelected, i), std::max(_firstSelected, i)};
            }
            if(ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
               _selectedFiles.x == _selectedFiles.y)
                _selectedFiles = {i, i};
            ImGui::SameLine(0, 0);
            ImVec4 iconColor = {1, 0, 0, 1};

            switch(type)
            {
                case FileType::directory:
                    iconColor = {0.8, 0.8, 0.8, 1};
                    break;
                case FileType::normal:
                    iconColor = {1, 1, 1, 1};
                    break;
                case FileType::source:
                    iconColor = {0, .80, 1, 1};
                    break;
                case FileType::asset:
                    iconColor = {0, 1, .25, 1};
                    break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
            ImGui::Text("%s", getIcon(file));
            ImGui::PopStyleColor();
            ImGui::SameLine();
            std::string filename = file.path().filename().string();
            ImGui::Text("%s", filename.c_str());
            ImGui::PopID();
        }
        ImGui::PopStyleVar(1);

        if(!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(0))
            _selectedFiles = {-1, -1};

        if(newDirectory)
            setDirectory(newDirectory);
    }
    assert(_currentDir);
    if(_allowEdits)
    {

        if(ImGui::BeginPopupContextWindow("directoryActions"))
        {
            if(ImGui::BeginMenu("Create New"))
            {
                if(ImGui::Selectable(ICON_FA_FOLDER " Directory"))
                    _ui.openPopup(std::make_unique<CreateDirectoryPopup>(*this));
                if(ImGui::Selectable(ICON_FA_CUBE " Chunk"))
                    _ui.openPopup(std::make_unique<CreateAssetPopup>(*this, AssetType::chunk));
                if(ImGui::Selectable(ICON_FA_SCROLL " Script"))
                    _ui.openPopup(std::make_unique<CreateAssetPopup>(*this, AssetType::script));
                if(ImGui::Selectable(ICON_FA_SPRAY_CAN_SPARKLES " Material"))
                    _ui.openPopup(std::make_unique<CreateAssetPopup>(*this, AssetType::material));
                if(ImGui::Selectable(ICON_FA_FIRE " Shader"))
                    _ui.openPopup(std::make_unique<CreateAssetPopup>(*this, AssetType::shader));
                ImGui::EndMenu();
            }
            if(ImGui::Selectable(ICON_FA_FOLDER " Show In File Browser"))
            {

#ifdef _WIN32
                const std::string fileBrowser = "explorer \"";
#elif __unix__
                const std::string fileBrowser = "xdg-open \"";
#endif
                std::string command = (fileBrowser + currentDirectory().string() + "\"");
                Logging::pushLog("running " + command, Logging::LogLevel::log);
                system(command.c_str());
            }
            if(_selectedFiles.x != -1)
            {
                ImGui::Separator();
                if(_selectedFiles.x == _selectedFiles.y)
                {
                    if(getFileType(_contents[_selectedFiles.x]) == FileType::source &&
                       ImGui::Selectable(ICON_FA_UP_RIGHT_FROM_SQUARE " Edit"))
                    {
#ifdef _WIN32
                        const std::string osCommand = R"(start "" ")";
#elif __unix__
                        const std::string osCommand = "xdg-open \"";
#endif
                        auto path = currentDirectory() / _contents[_selectedFiles.x].path();
                        system((osCommand + path.string() + "\"").c_str());
                    }
                }

                if(ImGui::Selectable(ICON_FA_TRASH " Delete"))
                {
                    Runtime::warn("TODO delete assets from project file");
                    for(size_t i = _selectedFiles.x; i <= _selectedFiles.y; ++i)
                    {
                        FileManager::deleteFile(_contents[i]);
                    }
                    reloadCurrentDirectory();
                }
            }
            ImGui::EndPopup();
        }
    }
}

std::filesystem::path AssetBrowserWidget::currentDirectory()
{
    return _rootPath / _currentDir->path();
}

void AssetBrowserWidget::displayFullBrowser()
{
    ImGui::TextDisabled("/%s", _currentDir->path().string().c_str());

    if(_allowEdits)
    {
        /* When an asset is selected, we create an AssetEditorContext object, and that creates/destroys
         * entities. This should really only be done on the main thread, as otherwise we might get race
         * conditions. As we sometimes need to request an asset, the responses may come in on another thread
         * and we need to switch back to main*/
        while(!_mainThreadActions.empty())
            _mainThreadActions.pop_front()();
    }

    if(_currentDir != _root.get())
    {
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 10);
        if(ImGui::Selectable(ICON_FA_ARROW_LEFT))
            setDirectory(_currentDir->parent);
    };

    ImGui::Separator();

    if(ImGui::BeginTable("window split",
                         2,
                         ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInnerV,
                         ImGui::GetContentRegionAvail()))
    {
        ImGui::TableNextColumn();
        ImGui::BeginChild("Directory Tree", {0, 0}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        displayDirectoryTree();
        ImGui::EndChild();
        ImGui::TableNextColumn();
        ImGui::BeginChild("Directory Contents", {0, 0}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        displayFiles();
        ImGui::EndChild();
        ImGui::EndTable();
    }
}

void AssetBrowserWidget::reloadCurrentDirectory()
{
    setDirectory(_currentDir);
}

const char* AssetBrowserWidget::getIcon(const std::filesystem::path& path)
{
    auto ext = path.extension();
    if(ext == "")
        return ICON_FA_FOLDER;
    if(ext == ".shader")
        return ICON_FA_FIRE;
    if(ext == ".material")
        return ICON_FA_SPRAY_CAN_SPARKLES;
    if(ext == ".script")
        return ICON_FA_SCROLL;
    if(ext == ".png")
        return ICON_FA_FILE_IMAGE;
    if(ext == ".image")
        return ICON_FA_IMAGE;
    if(ext == ".chunk")
        return ICON_FA_CUBE;
    if(ext == ".assembly")
        return ICON_FA_BOXES_STACKED;
    if(ext == ".gltf" || ext == ".glb")
        return ICON_FA_TABLE_CELLS;
    if(ext == ".bin")
        return ICON_FA_BOX_ARCHIVE;
    if(ext == ".vert" || ext == ".frag" || ext == ".lua")
        return ICON_FA_CODE;
    return ICON_FA_FILE;
}

AssetBrowserWidget::FileType AssetBrowserWidget::getFileType(const std::filesystem::directory_entry& file)
{
    if(file.is_directory())
        return FileType::directory;
    if(file.is_regular_file())
    {
        auto ext = file.path().extension();
        if(ext == ".shader" || ext == ".assembly" || ext == ".chunk" || ext == ".material" || ext == ".image" ||
           ext == ".script")
            return FileType::asset;
        if(ext == ".gltf" || ext == ".glb" || ext == ".vert" || ext == ".frag" || ext == ".bin" || ext == ".png" ||
           ext == ".lua")
            return FileType::source;
        return FileType::normal;
    }
    return FileType::unknown;
}

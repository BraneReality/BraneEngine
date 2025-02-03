//
// Created by wirewhiz on 20/07/22.
//

#include "createAssetWindow.h"
#include "../widgets/assetSelectWidget.h"
#include "assets/assetManager.h"
#include "assets/types/materialAsset.h"
#include "assets/types/shaderAsset.h"
#include "fileManager/fileManager.h"
#include "graphics/shader.h"
#include "imgui_stdlib.h"

bool CreateAssetWindow::_spirvHelperCreated = false;

CreateAssetWindow::CreateAssetWindow(GUI& ui, Editor& editor, FileManager::Directory* startingDir)
    : GUIWindow(ui), _browser(ui, false)
{
    _name = "Create Asset";
    if(startingDir)
        _browser.setDirectory(startingDir);
}

void CreateAssetWindow::displayContent()
{
    if(_uploadContext && !_uploadContext->done)
        ImGui::BeginDisabled();
    const std::array<const char*, 8> types = {
        "select type", "component", "system", "mesh", "material", "texture", "shader", "assembly"};
    ImGui::InputText("Name", &_assetName, ImGuiInputTextFlags_AutoSelectAll);
    if(ImGui::BeginCombo("Type", _assetType.toString().c_str()))
    {
        for(size_t i = 1; i < types.size(); ++i)
        {
            if(ImGui::Selectable(types[i]))
                _assetType.set(types[i]);
        }
        ImGui::EndCombo();
    }

    if(!_selectingDirectory)
    {
        if(ImGui::Button("change"))
            _selectingDirectory = true;
        ImGui::SameLine();
        ImGui::Text("Directory: %ls", _browser.currentDirectory().c_str());
    }
    else
    {
        ImGui::BeginChild("Directory View",
                          {ImGui::GetWindowContentRegionMax().x, 500},
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        if(ImGui::Button("select"))
            _selectingDirectory = false;
        ImGui::SameLine();
        _browser.displayFullBrowser();
        ImGui::EndChild();
    }
    ImGui::Separator();
    bool noCreate =
        _assetType == AssetType::Type::none || _selectingDirectory || (_uploadContext && !_uploadContext->done);
    switch(_assetType.type())
    {
        case AssetType::none:
            ImGui::TextDisabled("Select asset type to create");
            break;
        case AssetType::assembly:
            if(ImGui::Button("Select gltf"))
                _importFile = FileManager::requestLocalFilePath("gltf", {"*.glb", "*.gltf"});
            ImGui::SameLine();
            ImGui::Text("Import from file: %s", _importFile.c_str());
            AssetSelectWidget::draw(_defaultMaterial, AssetType::material);
            if(_importFile.empty())
                noCreate = true;
            break;
        case AssetType::shader:
            if(ImGui::Button("Select glsl file"))
                _importFile = FileManager::requestLocalFilePath("glsl", {"*.frag", "*.vert", "*.geometry"});
            ImGui::SameLine();
            ImGui::Text("Import from file: %s", _importFile.c_str());
            if(_importFile.empty())
                noCreate = true;
            break;
        case AssetType::material:
            break;
        default:
            noCreate = true;
            ImGui::Text("Asset type %s not implemented", _assetType.toString().c_str());
    }
    ImGui::Separator();
    if(_uploadContext && !_uploadContext->done)
        ImGui::EndDisabled();
    ImGui::BeginDisabled(noCreate);
    if(ImGui::Button("Create"))
        createAsset();
    ImGui::EndDisabled();
    if(_uploadContext)
    {
        _uploadContext->update();
        if(!_uploadContext->done)
            ImGui::Text("Upload status: %s", _uploadContext->status.c_str());
        else
            ImGui::Text("Upload completed");
    }
}

void CreateAssetWindow::createAsset()
{
    auto* fs = Runtime::getModule<ServerFilesystem>();
    switch(_assetType.type())
    {
        case AssetType::assembly:
        {
            gltfLoader loader;
            if(!loader.loadFromFile(_importFile))
            {
                Runtime::error("Failed to read gltf file!");
                break;
            }
            AssemblyBuilder::AssemblyAssets assets;
            auto* am = Runtime::getModule<AssetManager>();
            try
            {
                assets =
                    AssemblyBuilder::buildAssembly(_assetName, loader, am->getAsset<MaterialAsset>(_defaultMaterial));
            }
            catch(const std::exception& e)
            {
                Runtime::error("Failed to parse gltf file! " + std::string(e.what()));
                break;
            }

            _uploadContext = std::make_unique<AssemblyUploadContext>();
            _uploadContext->directory = _browser.currentDirectory();
            assets.assembly->meshes.resize(assets.meshes.size());
            for(size_t i = 0; i < assets.meshes.size(); ++i)
            {
                fs->saveAsset(_browser.currentDirectory(), assets.meshes[i].get()).then([this, i](AssetID id) {
                    ((AssemblyUploadContext*)_uploadContext.get())->assembly->meshes[i] = id.string();
                    ((AssemblyUploadContext*)_uploadContext.get())->meshesUploaded++;
                });
            }
            ((AssemblyUploadContext*)_uploadContext.get())->assembly = std::move(assets.assembly);
            break;
        }
        case AssetType::shader:
        {
            if(!_spirvHelperCreated)
            {
                graphics::SpirvHelper::Init();
                _spirvHelperCreated = true;
            }
            _uploadContext = std::make_unique<AssetUploadContext>();
            _uploadContext->status = "compiling...";
            ThreadPool::enqueue([this, fs]() {
                ShaderAsset shaderAsset;
                shaderAsset.name = _assetName;
                std::string fileSuffix = _importFile.substr(_importFile.find_last_of('.'));
                VkShaderStageFlagBits stageFlags;
                if(fileSuffix == ".vert")
                {
                    stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                    shaderAsset.shaderType = ShaderType::vertex;
                }
                else if(fileSuffix == ".frag")
                {
                    stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    shaderAsset.shaderType = ShaderType::fragment;
                }
                else
                {
                    Runtime::error("Unknown shader file suffix: " + fileSuffix);
                    _uploadContext->status = "failed, see console for more details";
                    return;
                }
                std::vector<char> shaderCode;
                Runtime::getModule<FileManager>()->readFile(_importFile, shaderCode);
                if(!graphics::CompileGLSL(stageFlags, shaderCode.data(), shaderAsset.spirv))
                {
                    Runtime::error("Shader compilation failed!");
                    _uploadContext->status = "failed, see console for more details";
                    return;
                }

                fs->saveAsset(_browser.currentDirectory(), &shaderAsset).then([this](AssetID id) {
                    _uploadContext->status = "Uploaded!";
                    _uploadContext->done = true;
                });
            });
            break;
        }
        case AssetType::material:
        {
            MaterialAsset materialAsset;
            materialAsset.name = _assetName;
            _uploadContext = std::make_unique<AssetUploadContext>();
            _uploadContext->status = "creating...";
            fs->saveAsset(_browser.currentDirectory(), &materialAsset).then([this](AssetID id) {
                _uploadContext->status = "done!";
                _uploadContext->done = true;
            });
        }
    }
}

CreateAssetWindow::~CreateAssetWindow()
{
    if(_spirvHelperCreated)
    {
        graphics::SpirvHelper::Finalize();
        _spirvHelperCreated = false;
    }
}

void CreateAssetWindow::AssemblyUploadContext::update()
{
    if(!uploadingAssembly && meshesUploaded == assembly->meshes.size())
    {
        uploadingAssembly = true;
        auto* fs = Runtime::getModule<ServerFilesystem>();
        fs->saveAsset(directory, assembly.get()).then([this](AssetID id) { done = true; });
        status = "Uploading assembly";
    }
    else if(!uploadingAssembly)
    {
        status =
            "Uploading meshes (" + std::to_string(meshesUploaded) + "/" + std::to_string(assembly->meshes.size()) + ")";
    }
}

void CreateAssetWindow::AssetUploadContext::update() {}

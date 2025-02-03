//
// Created by wirewhiz on 20/07/22.
//

#ifndef BRANEENGINE_CREATEASSETWINDOW_H
#define BRANEENGINE_CREATEASSETWINDOW_H

#include "../assets/assemblyBuilder.h"
#include "../widgets/assetBrowserWidget.h"
#include "editorWindow.h"
#include "utility/asyncData.h"

class CreateAssetWindow : public GUIWindow
{
    struct AssetUploadContext
    {
        bool done = false;
        FileManager::Directory* directory;
        std::string status;

        virtual void update();

        virtual ~AssetUploadContext() = default;
    };

    struct AssemblyUploadContext : public AssetUploadContext
    {
        std::unique_ptr<Assembly> assembly;
        std::atomic<size_t> meshesUploaded = 0;
        AsyncData<AssetID> assemblyID;
        bool uploadingAssembly = false;

        void update() override;
    };

    static bool _spirvHelperCreated;
    bool _selectingDirectory = false;
    AssetBrowserWidget _browser;

    std::string _importFile = "";
    AssetID _defaultMaterial;

    std::string _assetName = "new_asset";
    AssetType _assetType;

    std::unique_ptr<AssetUploadContext> _uploadContext;

    void displayContent() override;

    void createAsset();

  public:
    CreateAssetWindow(GUI& ui, Editor& editor, FileManager::Directory* startingDir = nullptr);

    ~CreateAssetWindow();
};

#endif // BRANEENGINE_CREATEASSETWINDOW_H

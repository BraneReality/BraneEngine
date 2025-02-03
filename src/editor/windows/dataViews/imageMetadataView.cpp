#include "imageMetadataView.h"
#include "assets/assetManager.h"
#include "graphics/graphics.h"
#include "graphics/texture.h"
#include "runtime/runtime.h"
#include <imgui_impl_vulkan.h>

ImageMetadataView::ImageMetadataView(Shared<ImageAssetMetadata> metadata) : _metadata(metadata)
{
    _editor = Runtime::getModule<Editor>();
    Runtime::getModule<AssetManager>()
        ->fetchAsset<ImageAsset>(*metadata->exportId->value())
        .then([this](Shared<ImageAsset> image) {
        Runtime::log("loaded image preivew asset");
        _previewImageAsset = Some(image);
        auto* texture = Runtime::getModule<graphics::VulkanRuntime>()->getTexture(image->runtimeID);
        if(texture)
            _imagePreview = ImGui_ImplVulkan_AddTexture(
                texture->sampler(), texture->view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        else
            Runtime::error("Preview image was not loaded into the runtime!");
    }).onError([](std::string error) { Runtime::error(std::format("Failed to preview image: {}", error)); });
}

Result<void> ImageMetadataView::draw()
{
    const char* imageTypes[3] = {"color", "normal map", "linear"};
    ImageAsset::ImageType imageType = *_metadata->colorSpace->value();
    Option<ImageAsset::ImageType> newType = None();
    if(ImGui::BeginCombo("ImageType", imageTypes[imageType]))
    {
        if(ImGui::Selectable("color"))
            newType = Some(ImageAsset::color);
        if(ImGui::Selectable("normal map"))
            newType = Some(ImageAsset::normal);
        if(ImGui::Selectable("linear"))
            newType = Some(ImageAsset::linear);
        ImGui::EndCombo();
    }
    if(newType)
        _editor->actionManager().executeAction(_metadata->colorSpace->set(newType.value()));

    if(_imagePreview && _previewImageAsset)
    {
        float width = ImGui::GetContentRegionMax().x;
        float height = width / (float)_previewImageAsset.value()->size.x * _previewImageAsset.value()->size.y;
        ImGui::Image((ImTextureID)_imagePreview.value(), {width, height});
    }
    else if(_previewImageAsset)
    {
        ImGui::TextDisabled("Loading image preview...");
        auto* texture =
            Runtime::getModule<graphics::VulkanRuntime>()->getTexture(_previewImageAsset.value()->runtimeID);
        if(texture)
            _imagePreview = ImGui_ImplVulkan_AddTexture(
                texture->sampler(), texture->view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    return Ok<void>();
}

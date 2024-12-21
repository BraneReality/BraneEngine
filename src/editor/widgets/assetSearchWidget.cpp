//
// Created by wirewhiz on 07/08/22.
//

#include "assetSearchWidget.h"
#include "assets/assetManager.h"
#include "editor/editor.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "runtime/runtime.h"

AssetSearchWidget::AssetSearchWidget(AssetType type, size_t searchIncrement)
{
    _assetType = type;
    _searchIncrement = searchIncrement;
    _project = &Runtime::getModule<Editor>()->project();
    _searchResults = _project->searchAssets("", type);
}

bool AssetSearchWidget::draw()
{
    bool submit = false;
    if(ImGui::InputText("search", &_searchText)) {
        _selected = -1;
        _searchResults = _project->searchAssets(_searchText, _assetType);
    }
    ImGui::Separator();

    ImGui::BeginChild("asset search", {ImGui::GetContentRegionAvail().x, 500});
    for(size_t i = 0; i < _searchResults.size(); ++i) {
        auto& r = _searchResults[i];
        if(ImGui::Selectable(r.second.stem().string().c_str(), _selected == i, ImGuiSelectableFlags_DontClosePopups))
            _selected = i;
        if(ImGui::IsItemHovered()) {
            ImGui::SetTooltip("ID: %s", r.first.string().c_str());
            if(ImGui::IsMouseDoubleClicked(0))
                submit = true;
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::BeginDisabled(_selected < 0);
    if(ImGui::Button("Select"))
        submit = true;
    ImGui::EndDisabled();
    return submit;
}

const AssetID& AssetSearchWidget::currentSelected()
{
    assert(_selected >= 0);
    return _searchResults[_selected].first;
}

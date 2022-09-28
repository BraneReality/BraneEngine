//
// Created by eli on 5/21/2022.
//

#ifndef BRANEENGINE_ENTITIESWINDOW_H
#define BRANEENGINE_ENTITIESWINDOW_H

#include "editorWindow.h"
#include <memory>

class EditorAsset;
class EntityManager;
class Assembly;
class EntitiesWindow : public EditorWindow
{
    std::shared_ptr<EditorAsset> _asset;
	EntityManager* _em;
    size_t _selected = -1;
	void parentEntity(uint32_t entity, uint32_t newParent, uint32_t childIndex = 0);
    void displayAssetEntities(uint32_t index, bool isLastChild = true);
    void displayContent() override;
public:
    EntitiesWindow(GUI& ui, Editor& editor);
};


#endif //BRANEENGINE_ENTITIESWINDOW_H

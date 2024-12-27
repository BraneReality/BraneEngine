//
// Created by eli on 5/21/2022.
//

#ifndef BRANEENGINE_ENTITIESWINDOW_H
#define BRANEENGINE_ENTITIESWINDOW_H

#include <memory>
#include <vector>
#include "editorWindow.h"

class EditorAsset;

class EntityManager;

class Assembly;

class EntitiesWindow : public EditorWindow
{
    std::shared_ptr<EditorAsset> _asset;
    EntityManager* _em;
    size_t _selected = -1;
    int _deletedEntity = -1;
    int _createdEntityParent = -1;

    void childEntityIndexes(uint32_t entity, std::vector<uint32_t>& children);

    void displayAssetEntities(uint32_t index, bool isLastChild = true);

    void displayContent() override;

  public:
    EntitiesWindow(GUI& ui, Editor& editor);
};

#endif // BRANEENGINE_ENTITIESWINDOW_H

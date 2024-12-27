//
// Created by eli on 5/23/2022.
//

#ifndef BRANEENGINE_MEMORYMANAGERWINDOW_H
#define BRANEENGINE_MEMORYMANAGERWINDOW_H

#include "editorWindow.h"

class EntityManager;

class MemoryManagerWindow : public EditorWindow
{
    EntityManager* _em;

    void displayContent() override;

  public:
    MemoryManagerWindow(GUI& ui, Editor& editor);
};

#endif // BRANEENGINE_MEMORYMANAGERWINDOW_H

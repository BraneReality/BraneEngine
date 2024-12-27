//
// Created by eli on 8/22/2022.
//

#ifndef BRANEENGINE_EDITORWINDOW_H
#define BRANEENGINE_EDITORWINDOW_H

#include "ui/guiWindow.h"

class Editor;

class EditorWindow : public GUIWindow
{
  protected:
    Editor& _editor;

  public:
    EditorWindow(GUI& ui, Editor& editor);
};

#endif // BRANEENGINE_EDITORWINDOW_H

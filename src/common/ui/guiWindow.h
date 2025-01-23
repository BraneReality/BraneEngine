//
// Created by eli on 5/16/2022.
//

#ifndef BRANEENGINE_GUIWINDOW_H
#define BRANEENGINE_GUIWINDOW_H

#include <string>
#include "IconsFontAwesome6.h"
#include "imgui.h"
#include <unordered_map>

class GUI;

class GUIWindow
{
    static size_t _instanceCounter;
    size_t _instance;
    bool _open = true;
    bool _resetSize = true;

  protected:
    GUI& _ui;
    std::string _name;
    ImGuiWindowFlags _flags = ImGuiWindowFlags_None;

    virtual void displayContent() = 0;

  public:
    GUIWindow(GUI& ui);

    virtual ~GUIWindow() = default;

    virtual void draw();

    virtual void update();

    bool isOpen() const;

    void close();

    std::string name() const;

    void resizeDefault();
};

#endif // BRANEENGINE_GUIWINDOW_H

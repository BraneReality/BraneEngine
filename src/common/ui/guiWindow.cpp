//
// Created by eli on 5/16/2022.
//

#include "guiWindow.h"
#include "gui.h"
#include "runtime/runtime.h"

size_t GUIWindow::_instanceCounter = 0;

GUIWindow::GUIWindow(GUI& ui) : _ui(ui) { _instance = _instanceCounter++; }

void GUIWindow::update() {}

bool GUIWindow::isOpen() const { return _open; }

void GUIWindow::close() { _open = false; }

std::string GUIWindow::name() const { return _name + "##" + std::to_string(_instance); }

void GUIWindow::draw()
{
    if(_resetSize)
    {
        ImGui::SetNextWindowSize({500, 800});
        _resetSize = false;
    }
    if(ImGui::Begin(name().c_str(), &_open, _flags))
        displayContent();
    ImGui::End();
}

void GUIWindow::resizeDefault() { _resetSize = true; }

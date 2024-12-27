//
// Created by eli on 6/2/2022.
//

#ifndef BRANEENGINE_INPUT_H
#define BRANEENGINE_INPUT_H

#include "graphics/window.h"
#include "runtime/module.h"

class InputManager : public Module
{
    graphics::Window* _window;

  public:
    InputManager();

    void start() override;

    static const char* name();
};

#endif // BRANEENGINE_INPUT_H

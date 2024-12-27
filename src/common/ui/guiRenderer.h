//
// Created by eli on 7/9/2022.
//

#ifndef BRANEENGINE_GUIRENDERER_H
#define BRANEENGINE_GUIRENDERER_H

#include <graphics/renderer.h>

class GUI;

class GUIRenderer : public graphics::Renderer
{
    GUI* _gui;

  public:
    GUIRenderer(graphics::SwapChain& swapChain, GUI* gui);

    void render(VkCommandBuffer cmdBuffer) override;

    void rebuild() override;
};

#endif // BRANEENGINE_GUIRENDERER_H

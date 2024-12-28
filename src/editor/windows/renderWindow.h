//
// Created by eli on 5/21/2022.
//

#ifndef BRANEENGINE_RENDERWINDOW_H
#define BRANEENGINE_RENDERWINDOW_H

#include <memory>
#include <vector>
#include "ecs/entityID.h"
#include "editorWindow.h"
#include "glm/gtx/quaternion.hpp"
#include "imgui.h"
#include "ImGuizmo.h"
#include "vulkan/vulkan.h"

namespace graphics
{
    class RenderTexture;

    class SceneRenderer;

    class SwapChain;
} // namespace graphics

class Assembly;

class EditorAsset;

class RenderWindow : public EditorWindow
{
    graphics::RenderTexture* _texture = nullptr;
    graphics::SceneRenderer* _renderer;
    std::vector<VkDescriptorSet> _imGuiBindings;
    graphics::SwapChain* _swapChain;
    VkExtent2D _windowSize = {0, 0};
    bool _queueReload = false;
    uint64_t _frameCount = 0;
    bool _panning = false;
    ImVec2 _lastMousePos;
    bool _manipulating = false;
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE _gizmoMode = ImGuizmo::MODE::WORLD;

    std::shared_ptr<EditorAsset> _focusedAsset;

    struct AssemblyContex
    {
        Assembly* assembly;
        EntityID root;
    };

    std::vector<AssemblyContex> _assemblies;
    size_t _focusedAssetEntity;
    EntityID _focusedEntity;

    EntityID _cameraEntity;
    EntityID _lightEntity;

    float zoom = 5;
    glm::vec3 position = {0, 0, 0};
    glm::vec2 rotation = {24, -45};

    void displayContent() override;

  public:
    RenderWindow(GUI& ui, Editor& editor);

    ~RenderWindow();

    void update() override;

    void lookAt(glm::vec3 pos);

    glm::quat rotationQuat() const;
};

#endif // BRANEENGINE_RENDERWINDOW_H

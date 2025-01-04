//
// Created by eli on 5/21/2022.
//

#include "renderWindow.h"
#include "../assets/editorAsset.h"
#include "../editor.h"
#include "../editorEvents.h"
#include "graphics/camera.h"
#include "graphics/graphics.h"
#include "graphics/material.h"
#include "graphics/pointLightComponent.h"
#include "graphics/renderTarget.h"
#include "graphics/sceneRenderer.h"
#include "ui/gui.h"

#include "assets/assetManager.h"
#include "assets/types/meshAsset.h"
#include "ecs/nativeComponent.h"
#include "ecs/nativeTypes/assetComponents.h"
#include "editor/assets/assemblyReloadManager.h"
#include "editor/assets/types/editorAssemblyAsset.h"
#include "imgui_impl_vulkan.h"
#include <systems/transforms.h>

class RenderWindowAssetReady : public GUIEvent
{
    AssetID _id;

  public:
    RenderWindowAssetReady(const AssetID& id) : _id(id), GUIEvent("render window asset ready") {};

    const AssetID& id() const
    {
        return _id;
    }
};

RenderWindow::RenderWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _name = "Render";
    EntityManager& em = *Runtime::getModule<EntityManager>();
    graphics::VulkanRuntime& vkr = *Runtime::getModule<graphics::VulkanRuntime>();
    _renderer = vkr.createRenderer<graphics::SceneRenderer>(&vkr, &em);
    _renderer->setClearColor({.2, .2, .2, 1});
    _swapChain = vkr.swapChain();
    _ui.addEventListener<FocusAssetEvent>("focus asset", this, [this, &em](const FocusAssetEvent* event) {
        if(event->asset()->type() != AssetType::assembly)
            return;
        if(!_assemblies.empty())
        {
            auto* arm = Runtime::getModule<AssemblyReloadManager>();
            for(auto& e : _assemblies)
                arm->destroy(e.assembly, e.root);
            _assemblies.resize(0);
        }
        auto* am = Runtime::getModule<AssetManager>();
        _focusedAsset = event->asset();
        _focusedAssetEntity = -1;
        if(dynamic_cast<EditorAssemblyAsset*>(_focusedAsset.get()))
        {
            am->fetchAsset<Assembly>(AssetID::parse(_focusedAsset->data()["id"].asString()).ok())
                .then([this](Assembly* assembly) {
                _ui.sendEvent(std::make_unique<RenderWindowAssetReady>(assembly->id));
            }).onError([this](const std::string& error) {
                if(_focusedAsset)
                    Runtime::warn("Could not load " + _focusedAsset->name() + " to render: " + error);
                else
                    Runtime::warn("Could not load asset to render: " + error);
            });
        }
    });
    _ui.addEventListener<RenderWindowAssetReady>(
        "render window asset ready", this, [this, &em](const RenderWindowAssetReady* event) {
        auto* am = Runtime::getModule<AssetManager>();
        auto* arm = Runtime::getModule<AssemblyReloadManager>();
        auto* assembly = am->getAsset<Assembly>(event->id());
        assert(assembly);
        EntityID root = arm->instantiate(assembly);
        _assemblies.push_back(AssemblyContex{assembly, root});
    });
    _ui.addEventListener<FocusEntityAssetEvent>("focus entity asset", this, [this](const FocusEntityAssetEvent* event) {
        if(!_focusedAsset)
            return;
        auto* am = Runtime::getModule<AssetManager>();
        auto* arm = Runtime::getModule<AssemblyReloadManager>();
        auto* assembly = am->getAsset<Assembly>(AssetID::parse(_focusedAsset->data()["id"].asString()).ok());
        if(assembly)
        {
            _focusedAssetEntity = event->entity();
            if(event->entity() >= 0)
                _focusedEntity = arm->getEntity(assembly, 0, event->entity());
            else
                _focusedEntity = EntityID();
        }
    });
    _ui.addEventListener<FocusEntityEvent>(
        "focus entity", this, [this](const FocusEntityEvent* event) { _focusedEntity = event->id(); });
    _cameraEntity = em.createEntity(ComponentSet({Transform::def()->id, graphics::Camera::def()->id}));
    graphics::Camera camera;
    camera.fov = 45;
    em.setComponent(_cameraEntity, camera);

    _lightEntity = em.createEntity(ComponentSet({Transform::def()->id, PointLightComponent::def()->id}));
    Transform lightTransform;
    lightTransform.value = glm::translate(glm::mat4(1), {2, 2, -2});
    PointLightComponent pointLight{};
    pointLight.color = {1, 1, 1, 10};
    em.setComponent(_lightEntity, lightTransform);
    em.setComponent(_lightEntity, pointLight);

    ImGuizmo::AllowAxisFlip(false); // Maybe add this to the config at some point
}

RenderWindow::~RenderWindow()
{
    graphics::VulkanRuntime& vkr = *Runtime::getModule<graphics::VulkanRuntime>();
    vkDeviceWaitIdle(graphics::device->get());
    vkr.removeRenderer(_renderer);
    if(_texture)
        delete _texture;
}

void RenderWindow::update()
{
    _frameCount++;

    if(_queueReload)
    {
        vkDeviceWaitIdle(graphics::device->get());
        _renderer->clearTarget();
        if(!_imGuiBindings.empty())
            vkFreeDescriptorSets(
                graphics::device->get(), _ui.descriptorPool(), _imGuiBindings.size(), _imGuiBindings.data());
        _imGuiBindings.resize(0);
        delete _texture;
        _texture = nullptr;
        _queueReload = false;
    }

    if(!_texture && (_windowSize.width != 0 && _windowSize.height != 0))
    {
        _texture = new graphics::RenderTexture(_windowSize, true, *_swapChain);
        auto& images = _texture->images();
        _imGuiBindings.resize(0);
        for(auto image : images)
            _imGuiBindings.push_back(
                ImGui_ImplVulkan_AddTexture(_texture->sampler(), image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        _renderer->setTarget(_texture);
    }
}

void RenderWindow::displayContent()
{
    const bool hovered = ImGui::IsWindowHovered();
    ImGui::BeginDisabled(_gizmoOperation == ImGuizmo::OPERATION::TRANSLATE);
    if(ImGui::Button(ICON_FA_UP_DOWN_LEFT_RIGHT) || (hovered && !_panning && ImGui::IsKeyPressed(ImGuiKey_W, false)))
        _gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
    ImGui::EndDisabled();

    ImGui::SameLine(0, 0);
    ImGui::BeginDisabled(_gizmoOperation == ImGuizmo::OPERATION::ROTATE);
    if(ImGui::Button(ICON_FA_ROTATE) || (hovered && !_panning && ImGui::IsKeyPressed(ImGuiKey_E, false)))
        _gizmoOperation = ImGuizmo::OPERATION::ROTATE;
    ImGui::EndDisabled();

    ImGui::SameLine(0, 0);
    ImGui::BeginDisabled(_gizmoOperation == ImGuizmo::OPERATION::SCALE);
    if(ImGui::Button(ICON_FA_ARROWS_TO_DOT) || (hovered && !_panning && ImGui::IsKeyPressed(ImGuiKey_R, false)))
        _gizmoOperation = ImGuizmo::OPERATION::SCALE;
    ImGui::EndDisabled();

    ImGui::SameLine(0, 13);
    if(ImGui::Button(_gizmoMode == ImGuizmo::MODE::WORLD ? ICON_FA_GLOBE : ICON_FA_OBJECT_GROUP) ||
       (hovered && !_panning && ImGui::IsKeyPressed(ImGuiKey_Q, false)))
        _gizmoMode = _gizmoMode == ImGuizmo::MODE::WORLD ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;

    ImGui::SameLine(0, 13);
    if(ImGui::Button(ICON_FA_LIGHTBULB))
    {
        _focusedEntity = _lightEntity;
        _focusedAssetEntity = -1;
    }
    ImGui::SameLine();
    auto* em = Runtime::getModule<EntityManager>();
    auto* light = em->getComponent<PointLightComponent>(_lightEntity);
    if(ImGui::ColorButton("##ColorButton", *(ImVec4*)&light->color))
    {
        ImGui::OpenPopup("picker");
    }
    if(ImGui::BeginPopup("picker"))
    {
        ImGui::ColorPicker3("##picker", (float*)&light->color);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::DragFloat("Brightness", &light->color.a, 0.5, 0, 0, "%.3f");

    auto* cameraTransform = em->getComponent<Transform>(_cameraEntity);
    auto* camera = em->getComponent<graphics::Camera>(_cameraEntity);

    auto window = ImGui::GetContentRegionAvail();
    _windowSize = {static_cast<uint32_t>(glm::floor(glm::max((float)0, window.x))),
                   static_cast<uint32_t>(glm::floor(glm::max((float)0, window.y)))};
    if(_windowSize.width != 0 && _windowSize.height != 0)
    {
        if(_texture && (_windowSize.width != _texture->size().width || _windowSize.height != _texture->size().height))
        {
            _queueReload = true;
        }

        ImGuizmo::SetDrawlist();

        if(_texture)
        {

            ImGui::Image((ImTextureID)_imGuiBindings[_swapChain->currentFrame()], window);
            auto imgPos = ImGui::GetItemRectMin();
            auto imgSize = ImGui::GetItemRectSize();
            ImGuizmo::SetRect(imgPos.x, imgPos.y, imgSize.x, imgSize.y);
        }

        if(ImGui::IsWindowHovered() || _panning)
        {
            // TODO add in sensitivity settings for all these
            zoom = glm::max<float>(0, zoom - ImGui::GetIO().MouseWheel);

            bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            if(!_panning && mouseDown)
                _lastMousePos = ImGui::GetMousePos();
            _panning = mouseDown;
            if(_panning)
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                auto rq = rotationQuat();
                if(!ImGui::IsKeyDown(ImGuiKey_ModShift))
                {
                    rotation.x += (mousePos.y - _lastMousePos.y);
                    rotation.y += (mousePos.x - _lastMousePos.x);
                    glm::vec3 movementVec{};
                    if(ImGui::IsKeyDown(ImGuiKey_W))
                        movementVec.z += 1;
                    if(ImGui::IsKeyDown(ImGuiKey_S))
                        movementVec.z -= 1;
                    if(ImGui::IsKeyDown(ImGuiKey_A))
                        movementVec.x -= 1;
                    if(ImGui::IsKeyDown(ImGuiKey_D))
                        movementVec.x += 1;
                    if(ImGui::IsKeyDown(ImGuiKey_Q))
                        movementVec.y -= 1;
                    if(ImGui::IsKeyDown(ImGuiKey_E))
                        movementVec.y += 1;
                    // TODO use delta time
                    position += rq * movementVec * (1.0f / 144.0f);
                }
                else
                {
                    float mpp = (std::tan((3.14159f / 180.0f * camera->fov) / 2) * zoom * 2) / _windowSize.height;
                    position += rq * glm::vec3{0.0f, (mousePos.y - _lastMousePos.y) * mpp, 0.0f};
                    position += rq * glm::vec3{-(mousePos.x - _lastMousePos.x) * mpp, 0.0f, 0.0f};
                }
                _lastMousePos = ImGui::GetMousePos();
            }
        }

        if(ImGui::IsWindowHovered())
        {
            if(ImGui::IsKeyPressed(ImGuiKey_F) && (em->entityExists(_focusedEntity)))
                position = em->getComponent<Transform>(_focusedEntity)->value[3];
        }
        auto* em = Runtime::getModule<EntityManager>();
        if(em->entityExists(_focusedEntity) && em->hasComponent<Transform>(_focusedEntity))
        {
            glm::mat4 pt = camera->perspectiveMatrix({_windowSize.width, _windowSize.height});
            // Undo inversion since ImGuizmo won't expect it
            pt[1][1] *= -1;
            auto ct = glm::inverse(cameraTransform->value);

            auto objectTransform = Transforms::getGlobalTransform(_focusedEntity, *em);

            ImGuizmo::Manipulate((float*)&ct,
                                 (float*)&pt,
                                 _gizmoOperation,
                                 (_gizmoOperation != ImGuizmo::OPERATION::SCALE) ? _gizmoMode : ImGuizmo::MODE::LOCAL,
                                 (float*)&objectTransform);
            if(ImGuizmo::IsUsing())
            {
                _manipulating = true;
                Transforms::setGlobalTransform(_focusedEntity, objectTransform, *em);
            }
            else if(_manipulating)
            {
                _manipulating = false;
                if(_focusedAsset && _focusedAssetEntity != -1)
                {
                    auto* assembly = dynamic_cast<EditorAssemblyAsset*>(_focusedAsset.get());
                    assembly->data().beginMultiChange();
                    if(em->hasComponent<TRS>(_focusedEntity))
                    {
                        assembly->updateEntityComponent(_focusedAssetEntity,
                                                        em->getComponent<TRS>(_focusedEntity)->toVirtual());
                    }
                    if(em->hasComponent<Transform>(_focusedEntity))
                    {
                        assembly->updateEntityComponent(_focusedAssetEntity,
                                                        em->getComponent<Transform>(_focusedEntity)->toVirtual());
                    }
                    assembly->data().endMultiChange();
                }
            }
        }

        glm::quat rot = rotationQuat();
        cameraTransform->value =
            glm::translate(glm::mat4(1), position - rot * glm::vec3{0, 0, zoom}) * glm::toMat4(rot);
    }
}

void RenderWindow::lookAt(glm::vec3 pos)
{
    float yaw = atan2(-pos.x, -pos.z);
    float pitch = acos(glm::dot(glm::normalize(position), {0, 1, 0}));
    rotation = {glm::degrees(pitch) - 90, glm::degrees(yaw)};
}

glm::quat RenderWindow::rotationQuat() const
{
    return glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
           glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
}

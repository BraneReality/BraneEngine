//
// Created by eli on 5/23/2022.
//

#include "memoryManagerWindow.h"
#include "../editorEvents.h"
#include "ecs/entity.h"
#include "ui/gui.h"

MemoryManagerWindow::MemoryManagerWindow(GUI& ui, Editor& editor) : EditorWindow(ui, editor)
{
    _name = "Memory Manager";
    _em = Runtime::getModule<EntityManager>();
}

void MemoryManagerWindow::displayContent()
{
    size_t ecsMemory = 0;
    if(ImGui::CollapsingHeader(ICON_FA_TABLE_CELLS "Archetypes"))
    {
        for(auto& arch : _em->archetypes())
        {
            ecsMemory += arch.chunks().size() * Chunk::allocationSize();
            std::string name = "| ";
            for(auto& c : arch.componentDescriptions())
            {
                if(!c->name.empty())
                    name += c->name + " | ";
                else
                    name += std::to_string(c->id) + "(ID) | ";
            }
            name += " x" + std::to_string(arch.size());
            ImGui::Selectable(name.c_str());
        }
    }
    if(ImGui::CollapsingHeader("Scene Entities"))
    {
        _em->systems().runUnmanagedSystem("display entities", [this](SystemContext* ctx) {
            ImGui::Indent(16);
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 13);
            ComponentFilter namedEntities(ctx);
            namedEntities.addComponent(EntityIDComponent::def()->id, ComponentFilterFlags_Const);
            namedEntities.addComponent(EntityName::def()->id, ComponentFilterFlags_Const);
            _em->getEntities(namedEntities).forEachNative([this](uint8_t** components) {
                auto* id = EntityIDComponent::fromVirtual(components[0]);
                auto* name = EntityName::fromVirtual(components[1]);
                if(ImGui::Selectable((name->name.empty()) ? ("##" + std::to_string((size_t)name)).c_str()
                                                          : name->name.c_str()))
                    _ui.sendEvent(std::make_unique<FocusEntityEvent>(id->id));
            });
            ComponentFilter unnamedEntities(ctx);
            unnamedEntities.addComponent(EntityIDComponent::def()->id, ComponentFilterFlags_Const);
            unnamedEntities.addComponent(EntityName::def()->id, ComponentFilterFlags_Exclude);
            _em->getEntities(unnamedEntities).forEachNative([this](uint8_t** components) {
                auto* id = EntityIDComponent::fromVirtual(components[0]);
                if(ImGui::Selectable(("Unnamed " + std::to_string(id->id.id)).c_str()))
                    _ui.sendEvent(std::make_unique<FocusEntityEvent>(id->id));
            });
            ImGui::PopStyleVar();
            ImGui::Unindent(16);
        });
    }

    ImGui::Text("Estimated ECS Memory: %fMB", static_cast<float>(ecsMemory) / 1000000);
}

//
// Created by eli on 12/9/2022.
//

#include "scripting.h"

#include "ecs/entity.h"
#include "transforms.h"
#include "structDefinition.h"

ScriptManager::ScriptManager()
{
    _rt.setLinker(&_linker);
    /*addNativeComponent<EntityIDComponent>(em);
    addNativeComponent<EntityName>(em);
    addNativeComponent<Transform>(em);
    addNativeComponent<LocalTransform>(em);
    addNativeComponent<Children>(em);
    addNativeComponent<TRS>(em);
    addNativeComponent<MeshRendererComponent>(em);
    addNativeComponent<PointLightComponent>(em);
    addNativeComponent<graphics::Camera>(em);*/

    const auto* floatDef = BraneScript::getNativeTypeDef(BraneScript::ValueType::Float32);

    auto* vec2Def = new BraneScript::StructDef("vec2");
    vec2Def->addMemberVar("x", floatDef, offsetof(glm::vec2, x));
    vec2Def->addMemberVar("y", floatDef, offsetof(glm::vec2, y));
    _linker.addType(vec2Def);

    auto* vec3Def = new BraneScript::StructDef("vec3");
    vec3Def->addMemberVar("x", floatDef, offsetof(glm::vec3, x));
    vec3Def->addMemberVar("y", floatDef, offsetof(glm::vec3, y));
    vec3Def->addMemberVar("z", floatDef, offsetof(glm::vec3, z));
    _linker.addType(vec3Def);

    auto* vec4Def = new BraneScript::StructDef("vec4");
    vec4Def->addMemberVar("x", floatDef, offsetof(glm::vec4, x));
    vec4Def->addMemberVar("y", floatDef, offsetof(glm::vec4, y));
    vec4Def->addMemberVar("z", floatDef, offsetof(glm::vec4, z));
    vec4Def->addMemberVar("w", floatDef, offsetof(glm::vec4, w));
    _linker.addType(vec4Def);

    auto* quatDef = new BraneScript::StructDef("quat");
    quatDef->addMemberVar("x", floatDef, offsetof(glm::quat, x));
    quatDef->addMemberVar("y", floatDef, offsetof(glm::quat, y));
    quatDef->addMemberVar("z", floatDef, offsetof(glm::quat, z));
    quatDef->addMemberVar("w", floatDef, offsetof(glm::quat, w));
    _linker.addType(quatDef);

    _linker.addType(TRS::newTypeDef(&_linker));
}

BraneScript::Compiler ScriptManager::newCompiler()
{
    return BraneScript::Compiler(&_linker);
}

BraneScript::Script* ScriptManager::addScript(const BraneScript::IRScript* irScript)
{
    auto script = _rt.assembleScript(irScript);
    if(!script)
    {
        Runtime::error("Unable to load script: " + irScript->namespace_);
    }
    return script;
}

BraneScript::Linker& ScriptManager::linker()
{
    return _linker;
}

const char* ScriptManager::name()
{
    return "scriptManager";
}
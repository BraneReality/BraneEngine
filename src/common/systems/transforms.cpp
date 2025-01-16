//
// Created by eli on 6/29/2022.
//

#include "transforms.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

glm::mat4 TRS::toMat() const
{
    return glm::translate(glm::mat4(1), translation) * glm::scale(glm::mat4(1), scale) * glm::mat4_cast(rotation);
}

void TRS::fromMat(const glm::mat4& t)
{
    glm::vec3 skew;
    glm::vec4 per;
    glm::decompose(t, scale, rotation, translation, skew, per);
}

void Transforms::start()
{
    _em = Runtime::getModule<EntityManager>();
    _em->systems().addSystem("transform", std::make_unique<TransformSystem>());
}

void Transforms::setParent(EntityID entity, EntityID parent, EntityManager& em, bool keepOffset)
{
    assert(em.entityExists(entity) && em.entityExists(parent));
    assert(entity != parent);
    removeParent(entity, em, false);
    if(!em.hasComponent<LocalTransform>(entity))
        em.addComponent<LocalTransform>(entity);
    auto* localTransform = em.getComponent<LocalTransform>(entity);

    localTransform->parent = parent;

    if(!em.hasComponent<Children>(parent))
        em.addComponent<Children>(parent);

    Children* children = em.getComponent<Children>(parent);
    children->children.push_back(entity);

    if(!em.hasComponent<Transform>(entity))
        em.addComponent<Transform>(entity);
    auto* t = em.getComponent<Transform>(entity);
    if(!keepOffset)
    {
        localTransform->value = t->value;
    }
    else
    {
        auto parentTransform = getParentTransform(parent, em);
        localTransform->value = glm::inverse(parentTransform) * t->value;
    }
    em.markComponentChanged(entity, Transform::def()->id);
    em.markComponentChanged(entity, LocalTransform::def()->id);
    if(em.hasComponent<TRS>(entity))
        em.markComponentChanged(entity, TRS::def()->id);
}

void Transforms::removeParent(EntityID entity, EntityManager& em, bool removeLocalTransform)
{
    if(!em.hasComponent<LocalTransform>(entity))
        return;
    EntityID parent = em.getComponent<LocalTransform>(entity)->parent;
    if(removeLocalTransform)
        em.removeComponent(entity, LocalTransform::def()->id);
    if(!em.entityExists(parent))
        return;
    Children* children = em.getComponent<Children>(parent);

    if(children->children.size() == 1)
    {
        em.removeComponent<Children>(parent);
        return;
    }

    size_t eraseIndex = 0;
    for(auto& child : children->children)
    {
        if(child == entity)
        {
            children->children.erase(eraseIndex);
            break;
        }
        ++eraseIndex;
    }
}

void Transforms::updateTRSFromMatrix(EntityID entity, glm::mat4 value)
{
    TRS trs{};
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(value, trs.scale, trs.rotation, trs.translation, skew, perspective);
    _em->setComponent(entity, trs.toVirtual());
}

const char* Transforms::name()
{
    return "transforms";
}

void Transforms::destroyRecursive(EntityID entity, bool updateParentChildren)
{
    assert(_em->entityExists(entity));
    if(_em->hasComponent<Children>(entity))
    {
        auto* cc = _em->getComponent<Children>(entity);
        for(auto child : cc->children)
        {
            destroyRecursive(child, false);
        }
    }
    if(updateParentChildren && _em->hasComponent(entity, LocalTransform::def()->id))
    {
        VirtualComponent lt = _em->getComponent(entity, LocalTransform::def()->id);
        VirtualComponent cc = _em->getComponent(*lt.getVar<EntityID>(1), Children::def()->id);
        auto& children = *cc.getVar<inlineEntityIDArray>(0);
        for(int i = 0; i < children.size(); ++i)
        {
            if(children[i] == entity)
            {
                children.erase(i);
                _em->setComponent(*lt.getVar<EntityID>(1), cc);
                break;
            }
        }
    }
    _em->destroyEntity(entity);
}

glm::mat4 Transforms::getParentTransform(EntityID parent, EntityManager& em)
{
    if(!em.entityExists(parent))
        return glm::mat4(1);
    if(!em.hasComponent<LocalTransform>(parent))
    {
        if(em.hasComponent<Transform>(parent))
            return em.getComponent<Transform>(parent)->value;
        return glm::mat4(1);
    }
    auto* gt = em.getComponent<Transform>(parent);
    if(!gt->dirty)
        return gt->value;
    auto* lt = em.getComponent<LocalTransform>(parent);
    glm::mat4 pt = getParentTransform(lt->parent, em);
    gt->value = pt * lt->value;
    gt->dirty = false;
    return gt->value;
}

glm::mat4& Transforms::getGlobalTransform(EntityID entity, EntityManager& em)
{
    auto t = em.getComponent<Transform>(entity);
    if(!t->dirty || !em.hasComponent<LocalTransform>(entity))
        return t->value;

    auto lt = em.getComponent<LocalTransform>(entity);
    glm::mat4 pt = getParentTransform(lt->parent, em);
    t->value = pt * lt->value;
    t->dirty = false;
    return t->value;
}

void Transforms::setGlobalTransform(EntityID entity, const glm::mat4& t, EntityManager& em)
{
    if(!em.hasComponent<Transform>(entity))
        em.addComponent<Transform>(entity);
    auto tc = em.getComponent<Transform>(entity);
    tc->value = t;
    if(em.hasComponent<TRS>(entity))
    {
        auto trs = em.getComponent<TRS>(entity);

        if(!em.hasComponent<LocalTransform>(entity))
            trs->fromMat(t);
        else
        {
            glm::mat4 pt = getParentTransform(em.getComponent<LocalTransform>(entity)->parent, em);
            trs->fromMat(glm::inverse(pt) * t);
        }
        em.markComponentChanged(entity, TRS::def()->id);
        return;
    }
    if(em.hasComponent<LocalTransform>(entity))
    {
        auto lt = em.getComponent<LocalTransform>(entity);
        glm::mat4 pt = getParentTransform(lt->parent, em);
        lt->value = glm::inverse(pt) * t;
        em.markComponentChanged(entity, LocalTransform::def()->id);
        return;
    }
    setDirty(entity, em);
}

void Transforms::setDirty(EntityID entity, EntityManager& em)
{
    auto t = em.getComponent<Transform>(entity);
    if(t->dirty)
        return;
    em.getComponent<Transform>(entity)->dirty = true;
    if(em.hasComponent<Children>(entity))
    {
        for(auto& c : em.getComponent<Children>(entity)->children)
        {
            if(!em.entityExists(c))
            {
                Runtime::warn("Invalid child set on entity " + std::to_string(c.id));
                continue;
            }
            setDirty(c, em);
        }
    }
}

void TransformSystem::run(EntityManager& _em)
{
    // Update trs for TRS components on unparented entities
    ComponentFilter globalTRS(&_ctx);
    globalTRS.addComponent(EntityIDComponent::def()->id, ComponentFilterFlags_Const);
    globalTRS.addComponent(TRS::def()->id, ComponentFilterFlags_Changed);
    globalTRS.addComponent(Transform::def()->id);
    globalTRS.addComponent(LocalTransform::def()->id, ComponentFilterFlags_Exclude);

    _em.getEntities(globalTRS).forEachNative([&_em](uint8_t** components) {
        EntityIDComponent* idc = EntityIDComponent::fromVirtual(components[0]);
        TRS* trs = TRS::fromVirtual(components[1]);
        Transform* t = Transform::fromVirtual(components[2]);
        t->value = trs->toMat();
        t->dirty = true;
        Transforms::setDirty(idc->id, _em);
    });

    // Update trs on parented entities
    ComponentFilter localTRS(&_ctx);
    localTRS.addComponent(EntityIDComponent::def()->id, ComponentFilterFlags_Const);
    localTRS.addComponent(TRS::def()->id, ComponentFilterFlags_Changed);
    localTRS.addComponent(LocalTransform::def()->id);
    localTRS.addComponent(Transform::def()->id);

    _em.getEntities(localTRS).forEachNative([&_em](uint8_t** components) {
        EntityIDComponent* idc = EntityIDComponent::fromVirtual(components[0]);
        TRS* trs = TRS::fromVirtual(components[1]);
        LocalTransform* t = LocalTransform::fromVirtual(components[2]);
        Transform* gt = Transform::fromVirtual(components[3]);
        t->value = trs->toMat();
        Transforms::setDirty(idc->id, _em);
    });

    // Update transform for parented entities
    ComponentFilter localTransforms(&_ctx);
    localTransforms.addComponent(Transform::def()->id);
    localTransforms.addComponent(LocalTransform::def()->id, ComponentFilterFlags_Const);

    _em.getEntities(localTransforms).forEachNative([&_em](uint8_t** components) {
        auto* gt = Transform::fromVirtual(components[0]);
        auto* lt = LocalTransform::fromVirtual(components[1]);
        glm::mat4 pt = Transforms::getParentTransform(lt->parent, _em);

        gt->value = pt * lt->value;
        gt->dirty = false;
    });
}

glm::vec3 Transform::pos() const
{
    return value[3];
}

glm::quat Transform::rot() const
{
    return glm::quat_cast(value);
}

glm::vec3 Transform::scale() const
{
    return {glm::length(glm::vec3(value[0])), glm::length(glm::vec3(value[1])), glm::length(glm::vec3(value[2]))};
}

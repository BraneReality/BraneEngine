//
// Created by eli on 6/29/2022.
//

#ifndef BRANEENGINE_TRANSFORMS_H
#define BRANEENGINE_TRANSFORMS_H

#include "common/ecs/entity.h"
#include <runtime/module.h>

class Transform : public NativeComponent<Transform>
{
    REGISTER_MEMBERS_2("Transform", value, "value", dirty, "dirty");

  public:
    glm::mat4 value = glm::mat4(1);
    bool dirty = true;

    glm::vec3 pos() const;

    glm::quat rot() const;

    glm::vec3 scale() const;
};

class LocalTransform : public NativeComponent<LocalTransform>
{
    REGISTER_MEMBERS_2("Local Transform", value, "value", parent, "parent id");

  public:
    glm::mat4 value = glm::mat4(1);
    EntityID parent;
};

class TRS : public NativeComponent<TRS>
{
    REGISTER_MEMBERS_3("TRS", translation, "translation", rotation, "rotation", scale, "scale");

  public:
    glm::vec3 translation = {0, 0, 0};          // local translation
    glm::quat rotation = glm::quat(1, 0, 0, 0); // local rotation
    glm::vec3 scale = {1, 1, 1};                // local scale
    glm::mat4 toMat() const;

    void fromMat(const glm::mat4& t);
};

class Children : public NativeComponent<Children>
{
    REGISTER_MEMBERS_1("Children", children, "children");

  public:
    inlineEntityIDArray children;
};

class Transforms : public Module
{
    EntityManager* _em;

    void updateTRSFromMatrix(EntityID entity, glm::mat4 value);

  public:
    static void setParent(EntityID entity, EntityID parent, EntityManager& em, bool keepOffset = true);

    static void removeParent(EntityID entity, EntityManager& em, bool removeLocalTransform = true);

    void destroyRecursive(EntityID entity, bool updateParentChildren = true);

    static glm::mat4 getParentTransform(EntityID parent, EntityManager& em);

    static glm::mat4& getGlobalTransform(EntityID entity, EntityManager& em);

    static void setGlobalTransform(EntityID entity, const glm::mat4& t, EntityManager& em);

    static void setDirty(EntityID entity, EntityManager& em);

    void start() override;

    static const char* name();
};

class TransformSystem : public System
{
  public:
    void run(EntityManager& _em) override;
};

#endif // BRANEENGINE_TRANSFORMS_H

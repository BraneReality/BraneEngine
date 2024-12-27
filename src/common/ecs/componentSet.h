#pragma once

#include <cassert>
#include <cstddef>
#include <set>
#include <vector>
#include "assets/types/componentAsset.h"
#include <unordered_map>
#include <unordered_set>

// Class that always has components sorted
class ComponentSet
{
  private:
    std::unordered_set<ComponentID> _components;

  public:
    ComponentSet() = default;

    ComponentSet(const std::vector<ComponentID>& components);

    void add(ComponentID component);

    void remove(ComponentID component);

    bool contains(ComponentID component) const;

    bool contains(const ComponentSet& subset) const;

    size_t size() const;

    bool operator==(const ComponentSet&) const;

    std::unordered_set<ComponentID>::const_iterator begin() const;

    std::unordered_set<ComponentID>::const_iterator end() const;
};

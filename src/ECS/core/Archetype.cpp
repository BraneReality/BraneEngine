#include "Archetype.h"

VirtualArchetype::VirtualArchetype(const std::vector<ComponentDefinition*>& componentDefs)
{
	for (size_t i = 0; i < componentDefs.size(); i++)
	{
		components.push_back(VirtualComponentVector(componentDefs[i]));
	}
}

bool VirtualArchetype::hasComponent(ComponentID component) const
{
	for (size_t i = 0; i < components.size(); i++)
	{
		if (components[i].def()->id() == component)
			return true;
	}
	return false;
}

bool VirtualArchetype::hasComponents(const std::vector<ComponentID>& comps) const
{
	if(components.size() < comps.size())
		return false;
	size_t aIndex = 0;
	size_t bIndex = 0;
	while (true)
	{
		if (components[aIndex].def()->id() != comps[bIndex])
			++aIndex;
		else
			++bIndex;
		if (components.size() - aIndex < comps.size() - bIndex)
			return false;
		if (aIndex == components.size())
			return false;
		if (bIndex == comps.size())
			return true;
	}
}

const VirtualComponentVector* VirtualArchetype::getComponentVector(ComponentID component) const
{
	for (size_t i = 0; i < components.size(); i++)
	{
		if (components[i].def()->id() == component)
			return &components[i];
	}
	assert(false); // could not find component vector
	return nullptr;
}

bool VirtualArchetype::isChildOf(const VirtualArchetype* parent, ComponentID& connectingComponent) const
{
	assert(components.size() + 1 == parent->components.size()); //Make sure this is a valid comparason

	byte missCount = 0;
	size_t lastFound = 0;
	// The parent component will always have more components and we want to test all of them, so use parent components for base loop
	for (size_t pIndex = 0; pIndex < parent->components.size(); pIndex++)
	{
		for (size_t cIndex = lastFound; cIndex < components.size(); cIndex++)
		{
			if (parent->components[pIndex].def()->id() != components[cIndex].def()->id())
			{
				// If this is a child there will be one miss, any other value and it is not a child
				if (++missCount == 2)
					return false;
				connectingComponent = parent->components[pIndex].def()->id();
			}
			else
			{
				lastFound = cIndex + 1;
				// We found the component, move on to the next one
				break;
			}
		}
	}
	if (missCount == 0 && parent->components[parent->components.size() - 1].def()->id() != components[components.size() - 1].def()->id())
	{
		connectingComponent = parent->components[parent->components.size() - 1].def()->id();
		++missCount;
	}
	assert(missCount == 1);
	return true;
}


std::vector<ComponentDefinition*> VirtualArchetype::getComponentDefs()
{
	std::vector<ComponentDefinition*> componentDefs;
	for (size_t i = 0; i < components.size(); i++)
	{
		componentDefs.push_back(components[i].def());
	}
	return componentDefs;
}

std::shared_ptr<ArchetypeEdge> VirtualArchetype::getEdge(ComponentID component)
{
	for (size_t i = 0; i < _edges.size(); i++)
	{
		if (component == _edges[i]->component)
			return _edges[i];
	}
	return std::shared_ptr<ArchetypeEdge>();
}

void VirtualArchetype::addEdge(std::shared_ptr<ArchetypeEdge> edge)
{
	_edges.push_back(edge);
}

size_t VirtualArchetype::createEntity()
{
	size_t index;
	index = components[0].size();   
	for (size_t i = 0; i < components.size(); i++)
	{
		components[i].pushEmpty();
	}
	return index;
}

size_t VirtualArchetype::copyEntity(VirtualArchetype* source, size_t index)
{
	size_t newIndex = createEntity();
	size_t lastFound = 0;
	for (size_t i = 0; i < components.size(); i++)
	{
		size_t localIndex = 0;
		while (localIndex < source->components.size() && source->components[localIndex].def()->id() != components[i].def()->id())
			++localIndex;
		// If the index is out of range the component was not found on the archetype we are copying from
		if (localIndex < source->components.size())
		{
			components[i].copy(source->components[localIndex], index, newIndex);
			lastFound = localIndex + 1;
		}
	}
	return newIndex;
}

void VirtualArchetype::swapRemove(size_t index)
{
	for (size_t i = 0; i < components.size(); i++)
	{
		assert(0 <= index && index < components[i].size());
		components[i].swapRemove(index);
	}
}

void VirtualArchetype::runSystem(VirtualSystem* vs, VirtualSystemConstants* constants)
{
	std::vector<ComponentID> requiredComponents = vs->requiredComponents();
	std::vector<VirtualComponentVector*> comps;
	size_t rIndex = 0;
	for (size_t i = 0; i < components.size(); i++)
	{
		if (requiredComponents[rIndex] == components[i].def()->id())
		{
			comps.push_back(&components[i]);
			++rIndex;
		}
	}
	assert(requiredComponents.size() == comps.size());

	std::vector<byte*> currentComponents;
	currentComponents.resize(comps.size());
	for (size_t i = 0; i < components[0].size(); i++)
	{
		for (size_t c = 0; c < comps.size(); c++)
		{
			currentComponents[c] = comps[c]->getComponentData(i);
		}
		vs->run(currentComponents, constants);
	}
}

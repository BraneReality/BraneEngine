#include "Entity.h"

VirtualArchetype* EntityManager::makeArchetype(std::vector<ComponentDefinition*>& cdefs)
{
	assert(cdefs.size() > 0);
	size_t numComps = cdefs.size();

	// Sort our VirtualStructDefinitions so that we always have a consistant order
	
	std::sort(cdefs.begin(), cdefs.end(), [](const ComponentDefinition* a, const ComponentDefinition* b){
		return a->id() < b->id();
	});
	
	// We want to keep archetypes of the same size in groups
	// This means we keep each size in a different vector

	// Make sure we have enough vectors
	if (numComps >= _archetypes.size())
		_archetypes.resize(numComps);

	size_t newIndex = _archetypes[numComps - 1].size();
	_archetypes[numComps - 1].push_back(std::make_unique<VirtualArchetype>(cdefs));

	VirtualArchetype* newArch = _archetypes[numComps - 1][newIndex].get();
	
	// find edges to other archetypes lower then this one
	if (numComps > 1)
	{
		for (size_t i = 0; i < _archetypes[numComps - 2].size(); i++)
		{
			ComponentID connectingComponent;
			if (_archetypes[numComps - 2][i]->isChildOf(newArch, connectingComponent))
			{
				std::shared_ptr<ArchetypeEdge> ae = std::make_shared<ArchetypeEdge>();
				ae->component = connectingComponent;
				ae->add = newArch;
				ae->remove = _archetypes[numComps - 2][i].get();
				newArch->addEdge(ae);
				_archetypes[numComps - 2][i]->addEdge(ae);
			}
		}
	}
	// find edges to other archetypes higher then this one
	if (_archetypes.size() > numComps)
	{
		for (size_t i = 0; i < _archetypes[numComps].size(); i++)
		{
			ComponentID connectingComponent;
			if (newArch->isChildOf(_archetypes[numComps][i].get(), connectingComponent))
			{
				std::shared_ptr<ArchetypeEdge> ae = std::make_shared<ArchetypeEdge>();
				ae->component = connectingComponent;
				ae->add = _archetypes[numComps][i].get();
				ae->remove = newArch;
				newArch->addEdge(ae);
				_archetypes[numComps][i]->addEdge(ae);
			}
		}
	}

	return newArch;
}

void EntityManager::regesterComponent(const ComponentDefinition& newComponent)
{
	assert(_components.find(newComponent.id()) == _components.end());
	// AParEntly std::unordered_map uses std::vector so whenever I add stuff to it, IT REALOCATES and RESIZES the memory Meaning all the pointers that I want to set to the data it contains just BREAK! but no longer... I WILL USE MORE POINTERS, THAT WILL DEFEAT THIS ANTI POINTER TIRANY!!!! this memory does not need to be contigougs like components so no Index maddness here, no maddnes at all... - WireWhiz, 2:40am 
	_components.insert({ newComponent.id(), std::make_unique<ComponentDefinition>(newComponent)});
}

void EntityManager::deregesterComponent(ComponentID component)
{
#ifdef DEBUG
	assert(_components.find(component) != _components.end());
#endif
	_components.erase(component);
}

VirtualArchetype* EntityManager::getArcheytpe(const std::vector<ComponentID>& components)
{
	size_t numComps = components.size();
	assert(numComps > 0);
	std::vector<std::unique_ptr<VirtualArchetype>>& arches = _archetypes[numComps - 1];
	for (size_t a = 0; a < arches.size(); a++)
	{
		assert(arches[a] -> components.size() == components.size());
		// Check every component, if one doesn't match it's not the archetype we want
		bool isMatch = true;
		for (size_t i = 0; i < arches[a]->components.size(); i++)
		{
			if (arches[a]->components[i].def()->id() != components[i])
			{
				isMatch = false;
				break;
			}
		}
		if(isMatch)
			return arches[a].get();
	}
	return nullptr;
}

EntityID EntityManager::createEntity()
{
	EntityID id;
	if (_unusedEntities.size() > 0)
	{
		id = _unusedEntities.front();
		_unusedEntities.pop();
	}
	else
	{
		id = _entities.size();
		_entities.push_back(EntityIndex());
	}
	
	EntityIndex& eIndex = _entities[id];
	eIndex.archetype = nullptr;
	eIndex.index = 0;
	eIndex.alive = true;

	return id;
}

void EntityManager::destroyEntity(EntityID entity)
{
	assert(0 <= entity < _entities.size());
	_entities[entity].alive = false;
	_unusedEntities.push(entity);
	VirtualArchetype* archetype = getEntityArchetype(entity);
	archetype->swapRemove(_entities[entity].index);
}

void EntityManager::runSystem(VirtualSystem* vs, VirtualSystemConstants* constants)
{
	std::vector<ComponentID> requiredComponents = vs->requiredComponents();
	for (size_t a = requiredComponents.size() - 1; a < _archetypes.size(); a++)
	{
		for (size_t i = 0; i < _archetypes[a].size(); i++)
		{
			if (_archetypes[a][i]->hasComponents(requiredComponents))
			{
				_archetypes[a][i]->runSystem(vs, constants);
			}
		}
	}
}

VirtualArchetype* EntityManager::getEntityArchetype(EntityID entity) const
{
	return _entities[entity].archetype;
}

bool EntityManager::hasArchetype(EntityID entity)
{
	return _entities[entity].archetype != nullptr;
}

VirtualComponentPtr EntityManager::getEntityComponent(EntityID entity, ComponentID component) const
{
	(getEntityArchetype(entity)->hasComponent(component));
	return getEntityArchetype(entity)->getComponentVector(component)->getComponent(_entities[entity].index);
}

void EntityManager::addComponent(EntityID entity, ComponentID component)
{
	
	VirtualArchetype* destArchetype = nullptr;
	size_t destArchIndex = 0;
	if (hasArchetype(entity))
	{
		VirtualArchetype* currentArchetype = getEntityArchetype(entity);
		assert(!currentArchetype->hasComponent(component)); // can't add a component that we already have
		// See if there's already an archetype we know about:
		std::shared_ptr<ArchetypeEdge> ae = currentArchetype->getEdge(component);
		if (ae != nullptr)
		{
			destArchetype = ae->add;
		}
		else
		{
			// Otherwise create one
			std::vector<ComponentDefinition*> compDefs = currentArchetype->getComponentDefs();
			compDefs.push_back(&*_components[component]);
			destArchetype = makeArchetype(compDefs);
			
		}
	}
	else
	{
		// the entity has no components, so we need to find or create a new one.
		if(_archetypes.size() > 0)
			for (size_t i = 0; i < _archetypes[0].size(); i++)
			{
				// All archetypes at index 0 have only one component, so checking them is fast.
				if (_archetypes[0][i]->components[0].def()->id() == component)
				{
					destArchetype = _archetypes[0][i].get();
				}
			}
		if (destArchetype == nullptr)
		{
			std::vector<ComponentDefinition*> compDefs;
			compDefs.push_back(&*_components[component]);
			destArchetype = makeArchetype(compDefs);
		}
		
	}
	
	size_t oldIndex = _entities[entity].index;
	
	// If this isn't an empty entity we need to copy and remove it
	if (hasArchetype(entity))
	{
		VirtualArchetype* arch = getEntityArchetype(entity);
		_entities[entity].index = destArchetype->copyEntity(arch, _entities[entity].index);
		arch->swapRemove(oldIndex);
	}
	else
	{
		_entities[entity].index = destArchetype->createEntity();
	}
	_entities[entity].archetype = destArchetype;
}

void EntityManager::removeComponent(EntityID entity, ComponentID component)
{
	VirtualArchetype* destArchetype = nullptr;
	size_t destArchIndex = 0;
	assert(hasArchetype(entity)); // Can't remove anything from an entity without any components
	VirtualArchetype* currentArchetype = getEntityArchetype(entity);
	assert(currentArchetype->hasComponent(component)); // can't remove a component that isn't there
	// See if there's already an archetype we know about:
	std::shared_ptr<ArchetypeEdge> ae = currentArchetype->getEdge(component);
	if (ae != nullptr)
	{
		assert(_entities[entity].archetype->components.size() >= 2); // Must be an archtype level beneath this one
		destArchetype = ae->remove;
	}
	else
	{
		// Otherwise create one
		std::vector<ComponentDefinition*> compDefs = currentArchetype->getComponentDefs();
		size_t componentIndex = 0;
		while (componentIndex < currentArchetype->components.size() && currentArchetype->components[componentIndex].def()->id() != component)
			++componentIndex;
		assert(componentIndex < currentArchetype->components.size());
		compDefs.erase(compDefs.begin() + componentIndex);
		if (compDefs.size() > 0)
		{
			assert(_entities[entity].archetype->components.size() >= 2); // Must be an archtype level beneath this one
			destArchIndex = _archetypes[_entities[entity].archetype->components.size() - 2].size();
			destArchetype = makeArchetype(compDefs);
		}
	}

	size_t oldIndex = _entities[entity].index;
	if (destArchetype != nullptr)
		_entities[entity].index = destArchetype->copyEntity(currentArchetype, _entities[entity].index);
	else
		_entities[entity].index = 0;
	currentArchetype->swapRemove(oldIndex);
	_entities[entity].archetype = destArchetype;
}

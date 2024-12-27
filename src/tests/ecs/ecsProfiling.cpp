//
// Created by eli on 8/28/2022.
//

#include <memory>
#include <random>
#include "assets/assetManager.h"
#include "ecs/entity.h"
#include "testing.h"
#include "unordered_set"
#include "utility/clock.h"

namespace std
{
    template<>
    struct hash<ComponentSet>
    {
        size_t operator()(const ComponentSet& componentSet) const
        {
            size_t h = 17;
            for(auto id : componentSet)
                h = h * 31 + hash<ComponentID>()(id);
            return h;
        }
    };
} // namespace std

void allPossibleComponentSets(const ComponentSet& set, std::unordered_set<ComponentSet>& results)
{
    if(results.count(set))
        return;
    results.insert(set);
    if(set.size() == 1)
        return;
    for(auto cID : set)
    {
        ComponentSet remaining = set;
        remaining.remove(cID);
        allPossibleComponentSets(remaining, results);
    }
}

std::unordered_set<ComponentSet> selectRandom(std::unordered_set<ComponentSet> sets, uint32_t count, uint64_t seed)
{
    std::unordered_set<ComponentSet> ret;
    ret.reserve(count);
    std::mt19937 gen(seed);
    std::discrete_distribution<> d({5, 10, 25, 60});
    for(uint32_t i = 0; i < count; ++i)
    {
        uint32_t index = d(gen) % sets.size();
        auto chosenSet = sets.begin();
        std::advance(chosenSet, index);
        ret.insert(*chosenSet);
        sets.erase(chosenSet);
    }
    return ret;
}

void printCallsPerFrame(size_t nanoseconds, size_t framerate)
{
    size_t timeInFrame = 1000000000 / framerate;
    std::cout << "Can be called " << timeInFrame / nanoseconds << " times in a frame, assuming " << framerate << "fps"
              << std::endl;
}

TEST(ECS_Profiling, QueryCreation_WorstCase)
{
    std::set<std::shared_ptr<ComponentDescription>> components;
    for(size_t i = 0; i < 12; ++i)
        components.insert(
            std::make_unique<ComponentDescription>(std::vector<VirtualType::Type>{VirtualType::virtualBool}));

    Runtime::init();

    Runtime::timeline()

        .addBlock("main");

    Runtime::addModule<EntityManager>();

    auto& em = *Runtime::getModule<EntityManager>();
    em.

        components()

            .

        registerComponent(EntityIDComponent::constructDescription());

    ComponentSet allComponents;
    for(auto& c : components)
    {
        em.

            components()

                .registerComponent(c.

                                   get()

                );
        allComponents.add(c->id);
    }

    std::unordered_set<ComponentSet> allSets;
    allPossibleComponentSets(allComponents, allSets);
    // Create one archetype for every possible combination of components.
    std::cout << "Running test with "
              << components.

                     size()

                     + 1
              << " components and "
              << allSets.

                 size()

              << " archetypes" << std::endl;
    for(const ComponentSet& set : allSets)
        em.createEntity(set);

    SystemContext ctx;
    {
        std::cout << "Query with 1 component took:\n";
        Stopwatch smallTime1;
        ComponentFilter smallFilter1(&ctx);
        smallFilter1.addComponent(0);
        auto smallSet1 = em.getEntities(smallFilter1);
        auto smallResult1 = smallTime1.time<std::chrono::nanoseconds>();
        std::cout << smallResult1 << " nanoseconds to find "
                  << smallSet1.

                     archetypeCount()

                  << " archetypes (EntityID, this is absolute worst case)\n";

        Stopwatch smallTime2;
        ComponentFilter smallFilter2(&ctx);
        smallFilter2.addComponent(8);
        auto smallSet2 = em.getEntities(smallFilter2);
        auto smallResult2 = smallTime2.time<std::chrono::nanoseconds>();
        std::cout << smallResult2 << " nanoseconds to find "
                  << smallSet2.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch smallTime3;
        ComponentFilter smallFilter3(&ctx);
        smallFilter3.addComponent(11);
        auto smallSet3 = em.getEntities(smallFilter3);
        auto smallResult3 = smallTime3.time<std::chrono::nanoseconds>();
        std::cout << smallResult3 << " nanoseconds to find "
                  << smallSet3.

                     archetypeCount()

                  << " archetypes\n";

        auto average = (smallResult1 + smallResult2 + smallResult3) / 3;
        std::cout << "average: " << average << " nanoseconds" << std::endl;
        printCallsPerFrame(average, 144);
        printCallsPerFrame(average, 120);
        printCallsPerFrame(average, 90);
        printCallsPerFrame(average, 60);
        std::cout << std::endl;
    }
    {
        std::cout << "Query with 3 components took:\n";
        Stopwatch mediumTime1;
        ComponentFilter medFilter(&ctx);
        for(auto id : {1, 2, 3})
            medFilter.addComponent(id);
        auto medSet1 = em.getEntities(medFilter);
        auto medResult1 = mediumTime1.time<std::chrono::nanoseconds>();
        std::cout << medResult1 << " nanoseconds to find "
                  << medSet1.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch mediumTime2;
        ComponentFilter medFilter2(&ctx);
        for(auto id : {1, 5, 10})
            medFilter2.addComponent(id);
        auto medSet2 = em.getEntities(medFilter2);
        auto medResult2 = mediumTime2.time<std::chrono::nanoseconds>();
        std::cout << medResult2 << " nanoseconds to find "
                  << medSet2.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch mediumTime3;
        ComponentFilter medFilter3(&ctx);
        for(auto id : {0, 9, 10})
            medFilter3.addComponent(id);
        auto medSet3 = em.getEntities(medFilter3);
        auto medResult3 = mediumTime3.time<std::chrono::nanoseconds>();
        std::cout << medResult3 << " nanoseconds to find "
                  << medSet3.

                     archetypeCount()

                  << " archetypes\n";

        auto average = (medResult1 + medResult2 + medResult3) / 3;
        std::cout << "average: " << average << " nanoseconds" << std::endl;
        printCallsPerFrame(average, 144);
        printCallsPerFrame(average, 120);
        printCallsPerFrame(average, 90);
        printCallsPerFrame(average, 60);
        std::cout << std::endl;
    }

    {
        std::cout << "Query with "
                  << components.

                     size()

                  << " components took:\n";
        Stopwatch largeTime;
        ComponentFilter largeFilter(&ctx);
        for(auto id : allComponents)
            largeFilter.addComponent(id);
        auto set = em.getEntities(largeFilter);
        auto largeResult = largeTime.time<std::chrono::nanoseconds>();

        std::cout << largeResult << " nanoseconds to find "
                  << set.

                     archetypeCount()

                  << " archetypes\n";
        printCallsPerFrame(largeResult, 144);
        printCallsPerFrame(largeResult, 120);
        printCallsPerFrame(largeResult, 90);
        printCallsPerFrame(largeResult, 60);
        std::cout << std::endl;
    }

    Runtime::cleanup();
}

TEST(ECS_Profiling, QueryCreation_PlausableCase)
{
    std::set<std::shared_ptr<ComponentDescription>> components;
    for(size_t i = 0; i < 12; ++i)
        components.insert(
            std::make_unique<ComponentDescription>(std::vector<VirtualType::Type>{VirtualType::virtualBool}));

    Runtime::init();

    Runtime::timeline()

        .addBlock("main");

    Runtime::addModule<EntityManager>();

    auto& em = *Runtime::getModule<EntityManager>();
    em.

        components()

            .

        registerComponent(EntityIDComponent::constructDescription());

    ComponentSet allComponents;
    for(auto& c : components)
    {
        em.

            components()

                .registerComponent(c.

                                   get()

                );
        allComponents.add(c->id);
    }

    std::unordered_set<ComponentSet> allSets;
    allPossibleComponentSets(allComponents, allSets);
    // Create one archetype for every possible combination of components.
    std::unordered_set<ComponentSet> randomSets = selectRandom(allSets, 650, 1234);
    for(const ComponentSet& set : randomSets)
        em.createEntity(set);

    std::cout << "Running test with "
              << components.

                     size()

                     + 1
              << " components and "
              << randomSets.

                 size()

              << " archetypes" << std::endl;

    SystemContext ctx;
    {
        std::cout << "Query with 1 component took:\n";
        Stopwatch smallTime1;
        ComponentFilter smallFilter1(&ctx);
        smallFilter1.addComponent(1);
        auto smallSet1 = em.getEntities(smallFilter1);
        auto smallResult1 = smallTime1.time<std::chrono::nanoseconds>();
        std::cout << smallResult1 << " nanoseconds to find "
                  << smallSet1.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch smallTime2;
        ComponentFilter smallFilter2(&ctx);
        smallFilter2.addComponent(8);
        auto smallSet2 = em.getEntities(smallFilter2);
        auto smallResult2 = smallTime2.time<std::chrono::nanoseconds>();
        std::cout << smallResult2 << " nanoseconds to find "
                  << smallSet2.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch smallTime3;
        ComponentFilter smallFilter3(&ctx);
        smallFilter3.addComponent(11);
        auto smallSet3 = em.getEntities(smallFilter3);
        auto smallResult3 = smallTime3.time<std::chrono::nanoseconds>();
        std::cout << smallResult3 << " nanoseconds to find "
                  << smallSet3.

                     archetypeCount()

                  << " archetypes\n";

        auto average = (smallResult1 + smallResult2 + smallResult3) / 3;
        std::cout << "average: " << average << " nanoseconds" << std::endl;
        printCallsPerFrame(average, 144);
        printCallsPerFrame(average, 120);
        printCallsPerFrame(average, 90);
        printCallsPerFrame(average, 60);
        std::cout << std::endl;
    }
    {
        std::cout << "Query with 3 components took:\n";
        Stopwatch mediumTime1;
        ComponentFilter medFilter(&ctx);
        for(auto id : {1, 2, 3})
            medFilter.addComponent(id);
        auto medSet1 = em.getEntities(medFilter);
        auto medResult1 = mediumTime1.time<std::chrono::nanoseconds>();
        std::cout << medResult1 << " nanoseconds to find "
                  << medSet1.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch mediumTime2;
        ComponentFilter medFilter2(&ctx);
        for(auto id : {1, 5, 10})
            medFilter2.addComponent(id);
        auto medSet2 = em.getEntities(medFilter2);
        auto medResult2 = mediumTime2.time<std::chrono::nanoseconds>();
        std::cout << medResult2 << " nanoseconds to find "
                  << medSet2.

                     archetypeCount()

                  << " archetypes\n";

        Stopwatch mediumTime3;
        ComponentFilter medFilter3(&ctx);
        for(auto id : {0, 9, 10})
            medFilter3.addComponent(id);
        auto medSet3 = em.getEntities(medFilter3);
        auto medResult3 = mediumTime3.time<std::chrono::nanoseconds>();
        std::cout << medResult3 << " nanoseconds to find "
                  << medSet3.

                     archetypeCount()

                  << " archetypes\n";

        auto average = (medResult1 + medResult2 + medResult3) / 3;
        std::cout << "average: " << average << " nanoseconds" << std::endl;
        printCallsPerFrame(average, 144);
        printCallsPerFrame(average, 120);
        printCallsPerFrame(average, 90);
        printCallsPerFrame(average, 60);
        std::cout << std::endl;
    }

    {
        std::cout << "Query with "
                  << components.

                     size()

                  << " components took:\n";
        Stopwatch largeTime;
        ComponentFilter largeFilter(&ctx);
        for(auto id : allComponents)
            largeFilter.addComponent(id);
        auto set = em.getEntities(largeFilter);
        auto largeResult = largeTime.time<std::chrono::nanoseconds>();

        std::cout << largeResult << " nanoseconds to find "
                  << set.

                     archetypeCount()

                  << " archetypes\n";
        printCallsPerFrame(largeResult, 144);
        printCallsPerFrame(largeResult, 120);
        printCallsPerFrame(largeResult, 90);
        printCallsPerFrame(largeResult, 60);
        std::cout << std::endl;
    }

    Runtime::cleanup();
}
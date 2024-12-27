#include <testing.h>
#include <ecs/entity.h>
#include <ecs/structMembers.h>
#include <utility/clock.h>

TEST(ECS, VirtualComponentTest)
{
    // Create a definition for our virtual struct/component

    std::vector<VirtualType::Type> variables = {
        VirtualType::virtualBool, VirtualType::virtualInt, VirtualType::virtualFloat};
    ComponentDescription vcd(variables);

    // Create a component using that definition
    VirtualComponent vc(&vcd);
    vc.setVar(0, true);
    vc.setVar(1, 69);
    vc.setVar<float>(2, 420);
    EXPECT_EQ(true, *vc.getVar<bool>(0));
    EXPECT_EQ(69, *vc.getVar<int>(1));
    EXPECT_EQ(420, *vc.getVar<float>(2));

    EXPECT_EQ(true, vc.readVar<bool>(0));
    EXPECT_EQ(69, vc.readVar<int>(1));
    EXPECT_EQ(420, vc.readVar<float>(2));
}

TEST(ECS, VirtualComponentComplexTypesTest)
{
    // Create a definition for our virtual struct/component

    std::vector<VirtualType::Type> variables = {VirtualType::virtualString};

    ComponentDescription vcd(variables);

    // Create a component using that definition
    VirtualComponent vc(&vcd);
    vc.setVar<std::string>(0, "Hello there! General Kenobi!");
    EXPECT_EQ("Hello there! General Kenobi!", *vc.getVar<std::string>(0));
}

TEST(ECS, ArchetypeTest)
{
    std::vector<VirtualType::Type> variables = {VirtualType::virtualString, VirtualType::virtualString};

    ComponentDescription a(variables);
    ComponentDescription b(variables);
    ComponentDescription c(variables);
    std::vector<const ComponentDescription*> components = {&a, &b, &c};

    std::shared_ptr<ChunkPool> cp = std::make_shared<ChunkPool>();
    Archetype arch(components, cp);

    EXPECT_EQ(arch.

              entitySize(),

              sizeof(std::string) * 6);
}

// Native component for testing
class TestNativeComponent : public NativeComponent<TestNativeComponent>
{
    REGISTER_MEMBERS_3("TestNativeComponent", var1, "var1", var2, "var2", var3, "var3")
  public:
    bool var1;
    int64_t var2;
    float var3;
};

// Classes for native system test
class TestNativeComponent2 : public NativeComponent<TestNativeComponent2>
{
    REGISTER_MEMBERS_1("TestNativeComponent2", var1, "var1")
  public:
    bool var1;
};

TEST(ECS, NativeComponentTest)
{
    TestNativeComponent nc;
    nc.

        constructDescription();

    nc.var1 = true;
    nc.var2 = 69;
    nc.var3 = 420;

    VirtualComponentView vc = nc.toVirtual();
    EXPECT_EQ(true, vc.readVar<bool>(0));
    EXPECT_EQ(69, vc.readVar<int>(1));
    EXPECT_EQ(420, vc.readVar<float>(2));

    vc.setVar(0, false);
    vc.setVar(1, 42);
    vc.setVar<float>(2, 42 * 2);

    EXPECT_EQ(false, vc.readVar<bool>(0));
    EXPECT_EQ(42, vc.readVar<int>(1));
    EXPECT_EQ(42 * 2, vc.readVar<float>(2));

    EXPECT_EQ(false, nc.var1);
    EXPECT_EQ(42, nc.var2);
    EXPECT_EQ(42 * 2, nc.var3);
}

// TODO Create test for ChunkComponentView

TEST(ECS, ChunkTest)
{
    // Create chunk
    std::unique_ptr<Chunk> c;
    std::shared_ptr<ChunkPool> cp = std::make_shared<ChunkPool>();
    *cp >> c;

    TestNativeComponent::constructDescription();

    TestNativeComponent2::constructDescription();

    std::vector<const ComponentDescription*> components = {TestNativeComponent::def(), TestNativeComponent2::def()};
    c->setComponents(components);

    EXPECT_EQ(c->

              maxCapacity(),
              c

                      ->_data.

                  size()

                  / (TestNativeComponent::def()->size() + TestNativeComponent2::def()->size()));
}

TEST(ECS, StructMembersTypesTest)
{
    std::vector<VirtualType::Type> members = STRUCT_MEMBER_TYPES_3(TestNativeComponent, var1, "", var2, "", var3, "");
    EXPECT_EQ(members.

              size(),

              3);

    EXPECT_EQ(members[0], VirtualType::virtualBool);
    EXPECT_EQ(members[1], VirtualType::virtualInt64);
    EXPECT_EQ(members[2], VirtualType::virtualFloat);
}

TEST(ECS, StructMembersOffsetsTest)
{
    std::vector<size_t> members = STRUCT_MEMBER_OFFSETS_3(TestNativeComponent, var1, "", var2, "", var3, "");
    EXPECT_EQ(members.

              size(),

              3);

    EXPECT_EQ(members[0], offsetof(TestNativeComponent, var1));
    EXPECT_EQ(members[1], offsetof(TestNativeComponent, var2));
    EXPECT_EQ(members[2], offsetof(TestNativeComponent, var3));
}

TEST(ECS, EntityManagerTest)
{
    Runtime::init();

    std::vector<VirtualType::Type> comps1 = {VirtualType::virtualBool};

    ComponentDescription vcd1(comps1);

    std::vector<VirtualType::Type> comps2 = {VirtualType::virtualFloat};
    ComponentDescription vcd2(comps2);

    std::vector<VirtualType::Type> comps3 = {VirtualType::virtualBool, VirtualType::virtualFloat};
    ComponentDescription vcd3(comps3);

    Runtime::timeline()

        .addBlock("main");
    EntityManager em;
    em.

        components()

            .

        registerComponent(EntityIDComponent::constructDescription());

    em.

        components()

            .registerComponent(&vcd1);
    em.

        components()

            .registerComponent(&vcd2);
    em.

        components()

            .registerComponent(&vcd3);

    // Create entity
    EntityID entity = em.createEntity();

    // Add component, this creates first archetype
    em.addComponent(entity, vcd1.id);
    // Get component, make sure we can read and write to it
    VirtualComponent comp = em.getComponent(entity, vcd1.id);
    comp.setVar<bool>(0, true);
    EXPECT_EQ(true, comp.readVar<bool>(0));
    em.setComponent(entity, comp);

    // Adds a second component, this creates a second archetype and copies the data over,
    em.addComponent(entity, vcd2.id);

    //  make sure the copied data is still valid
    comp = em.getComponent(entity, vcd1.id);
    EXPECT_EQ(true, comp.readVar<bool>(0));

    // Get second component, make sure we can read and write
    comp = em.getComponent(entity, vcd2.id);
    comp.setVar<float>(0, 42);
    EXPECT_EQ(42, comp.readVar<float>(0));
    em.setComponent(entity, comp);

    // make sure this throws no errors
    em.removeComponent(entity, vcd1.id);

    // Make sure that we still have the second component and can still read from it after the copy
    comp = em.getComponent(entity, vcd2.id);
    EXPECT_EQ(42, comp.readVar<float>(0));

    // just make sure these throw no errors
    em.addComponent(entity, vcd3.id);
    em.addComponent(entity, vcd1.id);
    em.removeComponent(entity, vcd2.id);
    em.removeComponent(entity, vcd3.id);
    em.removeComponent(entity, vcd1.id);

    // Make sure edges and automatic cleanup are working
    EXPECT_EQ(em._archetypes._archetypes.

              size(),

              4);
    EXPECT_EQ(em._archetypes._archetypes[0].

              size(),

              1);
    EXPECT_EQ(em._archetypes._archetypes[1].

              size(),

              0);
    EXPECT_EQ(em._archetypes._archetypes[2].

              size(),

              0);
    EXPECT_EQ(em._archetypes._archetypes[3].

              size(),

              0);

    em.destroyEntity(entity);
    EXPECT_EQ(em._archetypes._archetypes[0].

              size(),

              0);

    Runtime::cleanup();
}

// TODO component filter test

TEST(ECS, ForEachTest)
{
    Runtime::init();

    Runtime::timeline()

        .addBlock("main");
    EntityManager em;
    em.

        components()

            .

        registerComponent(EntityIDComponent::constructDescription());

    em.

        components()

            .

        registerComponent(TestNativeComponent::constructDescription());

    em.

        components()

            .

        registerComponent(TestNativeComponent2::constructDescription());

    std::array<EntityID, 100> entities;
    // Create 50 entities with one component, and 50 with two
    for(size_t i = 0; i < 100; i++)
    {
        EntityID e = em.createEntity();
        em.addComponent(e, TestNativeComponent::def()->id);
        // Create a new archetype
        if(i > 49)
            em.addComponent(e, TestNativeComponent2::def()->id);
        entities[i] = e;
    }

    EXPECT_EQ(em._archetypes._archetypes[0].

              size(),

              0);
    EXPECT_EQ(em._archetypes._archetypes[1].

              size(),

              1);

    // Set the variables on all the entities with the first component
    SystemContext ctx;
    ComponentFilter filter1(&ctx);
    filter1.

        addComponent(TestNativeComponent::def()

                         ->id);
    em.getEntities(filter1).forEachNative([&](byte* components[]) {
        TestNativeComponent* c1 = TestNativeComponent::fromVirtual(components[0]);
        c1->var1 = false;
        c1->var2 += 32;
        c1->var3 += 42;
    });

    // Set the variables on all the entities with two components
    ComponentFilter filter2(&ctx);
    filter2.

        addComponent(TestNativeComponent::def()

                         ->id);
    filter2.

        addComponent(TestNativeComponent2::def()

                         ->id);
    em.getEntities(filter2).forEachNative([&](byte* components[]) {
        TestNativeComponent* c1 = TestNativeComponent::fromVirtual(components[0]);
        c1->var1 = true;
        c1->var2 += 42;
        c1->var3 += 32;
        TestNativeComponent2* c2 = TestNativeComponent2::fromVirtual(components[1]);
        c2->var1 = true;
    });

    // Check that all the variables are what we set them too
    //
    // Checking the variables on all the entities with two components
    Stopwatch sw;
    em.getEntities(filter2).forEachNative([&](byte* components[]) {
        const TestNativeComponent* c1 = TestNativeComponent::fromVirtual(components[0]);
        const TestNativeComponent2* c2 = TestNativeComponent2::fromVirtual(components[1]);
        EXPECT_EQ(true, c1->var1);
        EXPECT_EQ(74, c1->var2);
        EXPECT_EQ(74, c1->var3);
        EXPECT_EQ(true, c2->var1);
    });
    long long time = sw.time<std::chrono::milliseconds>();
    std::cout << "For Each on 50 entities took: " << time << std::endl;

    std::cout << "Test done, checking values..." << std::endl;
    for(size_t i = 0; i < 100; i++)
    {
        if(i > 49)
        {
            // Entities with two
            TestNativeComponent* ts1 = em.getComponent<TestNativeComponent>(entities[i]);
            TestNativeComponent2* ts2 = em.getComponent<TestNativeComponent2>(entities[i]);

            EXPECT_EQ(true, ts1->var1) << "entity: " << i;
            EXPECT_EQ(74, ts1->var2) << "entity: " << i;
            EXPECT_EQ(74, ts1->var3) << "entity: " << i;
            EXPECT_EQ(true, ts2->var1) << "entity: " << i;
        }
        else
        {
            // Entites with one
            TestNativeComponent* ts1 = em.getComponent<TestNativeComponent>(entities[i]);
            EXPECT_EQ(false, ts1->var1) << "entity: " << i;
            EXPECT_EQ(32, ts1->var2) << "entity: " << i;
            EXPECT_EQ(42, ts1->var3) << "entity: " << i;
        }
    }

    Runtime::cleanup();
}

// Revive this test when we once again add threading back into ECS, for now, let it rest in peace
/*
TEST(ECS, ForEachParellelTest)
{
    Runtime::init();
    //Create two for each ID one for entities with one component, another for those with two

    std::vector<VirtualType::Type> variables(4, VirtualType::virtualUInt64);

    ComponentDescription counterComponent(variables);
    EntityManager em;
    em.components().registerComponent(EntityIDComponent::constructDescription());
    em.components().registerComponent(&counterComponent);

#ifdef _DEBUG
    size_t instances = 200000;
#else
    //Try to make it so this takes 1 second
    size_t instances = 20000000;
#endif

    em.createEntities(ComponentSet({counterComponent.id}), instances);
    Stopwatch sw;
    em.forEach({counterComponent.id}, [&](byte *components[]) {
        VirtualComponentView counter = VirtualComponentView(&counterComponent, components[0]);
        counter.setVar(0, 64);
    });
    long long time = sw.time();
    std::cout << "For Each took: " << time << std::endl;

    Stopwatch sw2;
    em.forEachParallel({counterComponent.id}, [&](byte *components[]) {
        VirtualComponentView counter = VirtualComponentView(&counterComponent, components[0]);
        counter.setVar(0, 420);
    }, instances / 12)->finish();
    time = sw2.time();
    std::cout << "For Each Parallel took: " << time << std::endl;

    for (size_t i = 0; i < instances; i++)
    {
        VirtualComponent c = em.getComponent(i, counterComponent.id);
        EXPECT_EQ(*c.getVar<size_t>(0), 420) << "entitiy: " << i << std::endl;
    }
    Runtime::cleanup();
}*/

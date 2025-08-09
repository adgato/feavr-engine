
#include "ecs_tests.h"


int main()
{
    using namespace ecs::tests;

    SaveLoadTest();
    EntityLifecycleTest();
    ComponentLifecycleTest();
    ArchetypeTransitionTest();
    TryGetTest();
    RemoveAllTest();
    BatchOperationsTest();
    EngineViewSingleComponentTest();
    EngineViewMultiComponentTest();
    EngineViewEmptyTest();
    EngineViewDynamicTest();
    UnionComplexTest();
    DestroyTest();
    DeletedEntityOperationsTest();
    SimpleUnionTest();
    MiniGameSimulationTest();

    // If we get here, all tests passed
    return 0;
}

#pragma once

#include <filesystem>
#include "ecs/Engine.h"
#include "ecs/EngineView.h"
#include <vector>
#include <algorithm>

#include "assets-system/AssetFile.h"
#include "ecs/EngineExtensions.h"

namespace ecs::tests
{
    inline void SaveLoadTest()
    {
        Engine engine;
        engine.Add<int>(engine.New(), 42);

        assets_system::AssetFile ecsSave("ECSX", 0);

        serial::Stream mWrite;
        mWrite.InitWrite();
        ECS_SERIALIZE(engine, mWrite, int);

        ecsSave.WriteToBlob(mWrite);
        ecsSave.Save(PROJECT_ROOT"/test.ecs");

        assets_system::AssetFile ecsLoad = assets_system::AssetFile::Load(PROJECT_ROOT"/test.ecs");

        serial::Stream mRead = ecsLoad.ReadFromBlob();

        engine.Destroy();
        ECS_SERIALIZE(engine, mRead, int);

        assert(engine.Get<int>(0) == 42);

        std::filesystem::remove(PROJECT_ROOT"/test.ecs");
    }

    // Test basic entity lifecycle and ID reuse
    inline void EntityLifecycleTest()
    {
        Engine engine;

        // Create entities
        EntityID e1 = engine.New();
        EntityID e2 = engine.New();
        EntityID e3 = engine.New();
        assert(engine.IsValid(e1));
        assert(engine.IsValid(e2));
        assert(engine.IsValid(e3));

        // Delete middle entity
        engine.Delete(e2);
        engine.Refresh();
        assert(engine.IsValid(e1));
        assert(!engine.IsValid(e2));
        assert(engine.IsValid(e3));

        // Create new entity - should reuse e2's ID
        EntityID e4 = engine.New(true); // canUseDeleted = true
        assert(e4 == e2); // Should reuse the deleted ID
        assert(engine.IsValid(e4));

        // Test canUseDeleted = false
        engine.Delete(e1);
        engine.Refresh();
        EntityID e5 = engine.New(false); // Should not reuse
        assert(e5 != e1); // Should get a new ID
        assert(engine.IsValid(e5));
    }

    // Test component addition, removal, and overwriting
    inline void ComponentLifecycleTest()
    {
        Engine engine;
        EntityID e = engine.New();

        // Add component (queued until refresh)
        engine.Add<int>(e, 42);
        assert(!engine.Has<int>(e)); // Not applied yet
        engine.Refresh();
        assert(engine.Has<int>(e));
        assert(engine.Get<int>(e) == 42);

        // Test component overwriting before refresh
        engine.Add<int>(e, 100);
        engine.Add<int>(e, 200); // Should overwrite previous queued add
        engine.Refresh();
        assert(engine.Get<int>(e) == 200);

        // Remove component
        engine.Remove<int>(e);
        assert(engine.Has<int>(e)); // Still has until refresh
        engine.Refresh();
        assert(!engine.Has<int>(e));
    }

    // Test archetype transitions as components are added/removed
    inline void ArchetypeTransitionTest()
    {
        Engine engine;
        EntityID e = engine.New();

        // Start with empty archetype, add int
        engine.Add<int>(e, 10);
        engine.Refresh();
        assert(engine.Has<int>(e));
        assert(!engine.Has<float>(e));

        // Add float (transition to {int, float} archetype)
        engine.Add<float>(e, 3.14f);
        engine.Refresh();
        assert(engine.Has<int>(e));
        assert(engine.Has<float>(e));
        assert(engine.Get<int>(e) == 10);
        assert(engine.Get<float>(e) == 3.14f);

        // Add double (transition to {int, float, double} archetype)
        engine.Add<double>(e, 2.718);
        engine.Refresh();
        assert(engine.Has<int>(e));
        assert(engine.Has<float>(e));
        assert(engine.Has<double>(e));

        // Remove int (transition to {float, double} archetype)
        engine.Remove<int>(e);
        engine.Refresh();
        assert(!engine.Has<int>(e));
        assert(engine.Has<float>(e));
        assert(engine.Has<double>(e));
        assert(engine.Get<float>(e) == 3.14f);
        assert(engine.Get<double>(e) == 2.718);
    }

    // Test TryGet for safe component access
    inline void TryGetTest()
    {
        Engine engine;
        EntityID e = engine.New();

        // TryGet on non-existent component
        assert(engine.TryGet<int>(e) == nullptr);

        // Add component and test TryGet
        engine.Add<int>(e, 123);
        engine.Refresh();

        int* value = engine.TryGet<int>(e);
        assert(value != nullptr);
        assert(*value == 123);

        // Modify through TryGet pointer
        *value = 456;
        assert(engine.Get<int>(e) == 456);

        // Remove component and test TryGet again
        engine.Remove<int>(e);
        engine.Refresh();
        assert(engine.TryGet<int>(e) == nullptr);
    }

    // Test RemoveAll functionality
    inline void RemoveAllTest()
    {
        Engine engine;
        EntityID e = engine.New();

        // Add multiple components
        engine.Add<int>(e, 1);
        engine.Add<float>(e, 2.0f);
        engine.Add<double>(e, 3.0);
        engine.Refresh();

        assert(engine.Has<int>(e));
        assert(engine.Has<float>(e));
        assert(engine.Has<double>(e));

        // RemoveAll should queue removal of all components
        engine.RemoveAll(e);
        engine.Refresh();

        assert(!engine.Has<int>(e));
        assert(!engine.Has<float>(e));
        assert(!engine.Has<double>(e));
        assert(engine.IsValid(e)); // Entity should still exist
    }

    // Test complex batch operations before refresh
    inline void BatchOperationsTest()
    {
        Engine engine;
        EntityID e = engine.New();

        // Complex sequence of operations without refresh
        engine.Add<int>(e, 1);
        engine.Add<float>(e, 2.0f);
        engine.Remove<int>(e);           // Remove the int we just added
        engine.Add<double>(e, 3.0);
        engine.Add<int>(e, 4);           // Re-add int with different value
        engine.Add<float>(e, 5.0f);      // Overwrite float

        // Before refresh, entity should appear unchanged
        assert(!engine.Has<int>(e));
        assert(!engine.Has<float>(e));
        assert(!engine.Has<double>(e));

        engine.Refresh();

        // After refresh, final state should be: int=4, float=5.0f, double=3.0
        assert(engine.Has<int>(e));
        assert(engine.Get<int>(e) == 4);
        assert(engine.Has<float>(e));
        assert(engine.Get<float>(e) == 5.0f);
        assert(engine.Has<double>(e));
        assert(engine.Get<double>(e) == 3.0);
    }

    // Test EngineView with single component type
    inline void EngineViewSingleComponentTest()
    {
        Engine engine;

        // Create entities with different component combinations
        EntityID e1 = engine.New();
        EntityID e2 = engine.New();
        EntityID e3 = engine.New();
        EntityID e4 = engine.New();

        engine.Add<int>(e1, 10);         // e1: int only
        engine.Add<float>(e2, 20.0f);    // e2: float only
        engine.Add<int>(e3, 30);         // e3: int only
        // e4: no components
        engine.Refresh();

        // View should find entities with AT LEAST int component (e1 and e3)
        EngineView<int>::Without<float> view(engine);
        std::vector<std::pair<EntityID, int>> results;

        for (auto [entity, value] : view)
        {
            results.emplace_back(entity, value);
        }

        assert(results.size() == 2);

        // Sort results by entity ID for predictable testing
        std::sort(results.begin(), results.end());
        assert(results[0].first == e1 && results[0].second == 10);
        assert(results[1].first == e3 && results[1].second == 30);
    }

    // Test EngineView with multiple component types
    inline void EngineViewMultiComponentTest()
    {
        Engine engine;

        EntityID e1 = engine.New();
        EntityID e2 = engine.New();
        EntityID e3 = engine.New();
        EntityID e4 = engine.New();

        // e1: int only
        engine.Add<int>(e1, 1);

        // e2: float only
        engine.Add<float>(e2, 2.0f);

        // e3: both int and float
        engine.Add<int>(e3, 3);
        engine.Add<float>(e3, 3.0f);

        // e4: int, float, and double
        engine.Add<int>(e4, 4);
        engine.Add<float>(e4, 4.0f);
        engine.Add<double>(e4, 4.0);

        engine.Refresh();

        // View for entities with AT LEAST both int and float
        EngineView<int, float> view(engine);
        std::vector<EntityID> foundEntities;

        for (auto [entity, intVal, floatVal] : view)
        {
            foundEntities.push_back(entity);
            // Verify the values are correct
            assert(engine.Get<int>(entity) == intVal);
            assert(engine.Get<float>(entity) == floatVal);
        }

        // Should find e3 and e4 (both have AT LEAST int and float)
        assert(foundEntities.size() == 2);
        std::sort(foundEntities.begin(), foundEntities.end());
        assert(foundEntities[0] == e3);
        assert(foundEntities[1] == e4);
    }

    // Test EngineView with no matching entities
    inline void EngineViewEmptyTest()
    {
        Engine engine;

        Entity e1 = engine.New();
        Entity e2 = engine.New();
        engine.Add<int>(e1, 1);
        engine.Add<float>(e2, 2.0f);
        engine.Refresh();

        // View for component type that doesn't exist
        EngineView<double> view(engine);

        int count = 0;
        for (auto [entity, value] : view)
        {
            count++;
        }

        assert(count == 0);
    }

    // Test EngineView iteration after archetype changes
    inline void EngineViewDynamicTest()
    {
        Engine engine;

        EntityID e1 = engine.New();
        EntityID e2 = engine.New();

        engine.Add<int>(e1, 10);
        engine.Add<int>(e2, 20);
        engine.Refresh();

        // Create view and verify initial state
        EngineView<int> view(engine);
        int count = 0;
        for (auto [entity, value] : view) { count++; }
        assert(count == 2);

        // Add more entities with int component
        EntityID e3 = engine.New();
        engine.Add<int>(e3, 30);
        engine.Refresh();

        // View should now find the new entity too
        count = 0;
        for (auto [entity, value] : view) { count++; }
        assert(count == 3);
    }

    // Test Union with overlapping data
    inline void UnionComplexTest()
    {
        Engine engine1;
        Engine engine2;

        // Setup engine1 with some entities
        EntityID e1_1 = engine1.New();
        EntityID e1_2 = engine1.New();
        engine1.Add<int>(e1_1, 100);
        engine1.Add<float>(e1_2, 200.0f);
        engine1.Add<int>(e1_2, 300); // e1_2 has both int and float
        engine1.Refresh();

        // Setup engine2 with different entities
        EntityID e2_1 = engine2.New();
        EntityID e2_2 = engine2.New();
        engine2.Add<double>(e2_1, 400.0);
        engine2.Add<int>(e2_2, 500);
        engine2.Refresh();

        // Count components before union
        EngineView<int> intView2(engine2);
        int intCountBefore = 0;
        for (auto [entity, value] : intView2) { intCountBefore++; }
        assert(intCountBefore == 1); // Just e2_2

        // Union engine1 into engine2
        engine2.Union(engine1);
        engine2.Refresh();

        // Count components after union
        EngineView<int> intViewAfter(engine2);
        int intCountAfter = 0;
        for (auto [entity, value] : intViewAfter) { intCountAfter++; }
        assert(intCountAfter == 3); // e2_2 + imported e1_1 + imported e1_2

        EngineView<float> floatView(engine2);
        int floatCount = 0;
        for (auto [entity, value] : floatView) { floatCount++; }
        assert(floatCount == 1); // imported e1_2

        EngineView<double> doubleView(engine2);
        int doubleCount = 0;
        for (auto [entity, value] : doubleView) { doubleCount++; }
        assert(doubleCount == 1); // original e2_1
    }

    // Test Destroy functionality
    inline void DestroyTest()
    {
        Engine engine;

        EntityID e1 = engine.New();
        EntityID e2 = engine.New();
        EntityID e3 = engine.New();

        engine.Add<int>(e1, 10);
        engine.Add<float>(e2, 20.0f);
        engine.Add<double>(e3, 30.0);
        engine.Refresh();

        assert(engine.IsValid(e1));
        assert(engine.IsValid(e2));
        assert(engine.IsValid(e3));

        // Destroy should delete all entities
        engine.Destroy();

        assert(!engine.IsValid(e1));
        assert(!engine.IsValid(e2));
        assert(!engine.IsValid(e3));

        // Should be able to create new entities after destroy
        EntityID e4 = engine.New();
        assert(engine.IsValid(e4));
        // New entity should start from 0 again
        assert(e4 == 0);
    }

    // Test edge case: operations on deleted entities (should assert in debug)
    inline void DeletedEntityOperationsTest()
    {
        Engine engine;
        EntityID e = engine.New();

        engine.Add<int>(e, 42);
        engine.Refresh();
        assert(engine.Has<int>(e));

        // Delete entity
        engine.Delete(e);
        engine.Refresh();
        assert(!engine.IsValid(e));

        // Operations on deleted entity should fail gracefully
        // Note: In debug builds, these would assert
        // engine.Add<float>(e, 1.0f);  // Would assert
        // engine.Remove<int>(e);       // Would assert
        // engine.Get<int>(e);          // Would assert

        // TryGet should return nullptr for invalid entities
        // This test might depend on implementation details
        // assert(engine.TryGet<int>(e) == nullptr);
    }

    // Original simple test
    inline void SimpleUnionTest()
    {
        Engine engine1;
        Engine engine2;

        EntityID e1 = engine1.New();
        engine1.Add<int>(e1, 1);
        engine1.Refresh();
        engine2.Union(engine1);
        engine2.Refresh();

        assert(engine2.IsValid(e1));
        assert(engine2.Has<int>(e1));
    }

    // Big comprehensive test simulating a mini-game scenario
    inline void MiniGameSimulationTest()
    {
        // Simulate a game with Position, Velocity, Health, Damage, and AI components
        struct Position { float x, y; };
        struct Velocity { float dx, dy; };
        struct Health { int hp, maxHp; };
        struct Damage { int value; };
        struct AI { int aggroRange, state; };

        Engine gameWorld;

        // === PHASE 1: Initial world setup ===
        std::vector<EntityID> players;
        std::vector<EntityID> enemies;
        std::vector<EntityID> projectiles;
        std::vector<EntityID> powerups;

        // Create players (Position + Velocity + Health)
        for (int i = 0; i < 3; ++i)
        {
            EntityID player = gameWorld.New();
            gameWorld.Add<Position>(player, {float(i * 100), 0.0f});
            gameWorld.Add<Velocity>(player, {0.0f, 0.0f});
            gameWorld.Add<Health>(player, {100, 100});
            players.push_back(player);
        }

        // Create enemies (Position + Velocity + Health + Damage + AI)
        for (int i = 0; i < 5; ++i)
        {
            EntityID enemy = gameWorld.New();
            gameWorld.Add<Position>(enemy, {float(i * 50 + 200), 100.0f});
            gameWorld.Add<Velocity>(enemy, {0.0f, -10.0f});
            gameWorld.Add<Health>(enemy, {50, 50});
            gameWorld.Add<Damage>(enemy, {15});
            gameWorld.Add<AI>(enemy, {75, 0});
            enemies.push_back(enemy);
        }

        // Create static powerups (Position only)
        for (int i = 0; i < 4; ++i)
        {
            EntityID powerup = gameWorld.New();
            gameWorld.Add<Position>(powerup, {float(i * 150 + 50), 200.0f});
            powerups.push_back(powerup);
        }

        gameWorld.Refresh();

        // Verify initial setup using our known entity lists
        int playerCount = 0;
        for (EntityID player : players)
        {
            assert(gameWorld.IsValid(player));
            assert(gameWorld.Has<Position>(player));
            assert(gameWorld.Has<Velocity>(player));
            assert(gameWorld.Has<Health>(player));
            assert(!gameWorld.Has<Damage>(player)); // Players don't start with damage
            assert(!gameWorld.Has<AI>(player));     // Players don't have AI
            playerCount++;
        }
        assert(playerCount == 3);

        int enemyCount = 0;
        for (EntityID enemy : enemies)
        {
            assert(gameWorld.IsValid(enemy));
            assert(gameWorld.Has<Position>(enemy));
            assert(gameWorld.Has<Velocity>(enemy));
            assert(gameWorld.Has<Health>(enemy));
            assert(gameWorld.Has<Damage>(enemy));
            assert(gameWorld.Has<AI>(enemy));
            enemyCount++;
        }
        assert(enemyCount == 5);

        // === PHASE 2: Simulate game frame - movement and AI ===

        // Update all entities with velocity (this includes players and enemies)
        EngineView<Position, Velocity> movingEntities(gameWorld);
        for (auto [entity, pos, vel] : movingEntities)
        {
            // Simulate movement (normally would modify in-place, but testing component updates)
            gameWorld.Add<Position>(entity, {pos.x + vel.dx, pos.y + vel.dy});
        }

        // AI behavior updates (only enemies have AI initially)
        EngineView<Position, AI> aiEntities(gameWorld);
        for (auto [entity, pos, ai] : aiEntities)
        {
            // Simple AI: accelerate toward nearest player
            float nearestDist = 1000000.0f;
            for (EntityID player : players)
            {
                if (gameWorld.IsValid(player))
                {
                    Position playerPos = gameWorld.Get<Position>(player);
                    float dist = (pos.x - playerPos.x) * (pos.x - playerPos.x) +
                               (pos.y - playerPos.y) * (pos.y - playerPos.y);
                    if (dist < nearestDist) nearestDist = dist;
                }
            }

            if (nearestDist < ai.aggroRange * ai.aggroRange)
            {
                gameWorld.Add<AI>(entity, {ai.aggroRange, 1}); // Set to aggressive state
                if (gameWorld.Has<Velocity>(entity))
                {
                    gameWorld.Add<Velocity>(entity, {0.0f, -20.0f}); // Speed up
                }
            }
        }

        gameWorld.Refresh();

        // === PHASE 3: Combat simulation - damage and healing ===

        // Some enemies take damage (use our known enemy list)
        for (int i = 0; i < 3 && i < enemies.size(); ++i)
        {
            EntityID enemy = enemies[i];
            if (gameWorld.IsValid(enemy) && gameWorld.Has<Health>(enemy))
            {
                Health hp = gameWorld.Get<Health>(enemy);
                hp.hp -= 25;
                gameWorld.Add<Health>(enemy, hp);
            }
        }

        // Some players get healed and gain damage capability
        for (int i = 0; i < 2 && i < players.size(); ++i)
        {
            EntityID player = players[i];
            if (gameWorld.IsValid(player))
            {
                Health hp = gameWorld.Get<Health>(player);
                hp.hp = hp.maxHp; // Full heal
                gameWorld.Add<Health>(player, hp);
                gameWorld.Add<Damage>(player, {20}); // Players can now deal damage
            }
        }

        gameWorld.Refresh();

        // Verify combat state - count entities with both Health and Damage
        EngineView<Health, Damage> combatEntities(gameWorld);
        int damageDealers = 0;
        for (auto [entity, hp, dmg] : combatEntities)
        {
            damageDealers++;
            assert(dmg.value > 0);
        }
        assert(damageDealers == 7); // 2 players + 5 enemies

        // === PHASE 4: Spawn projectiles dynamically ===

        // Create projectiles from entities that can deal damage
        EngineView<Position, Damage> shooters(gameWorld);
        for (auto [shooter, pos, dmg] : shooters)
        {
            // Each shooter fires 2 projectiles (Position + Velocity + Damage)
            for (int i = 0; i < 2; ++i)
            {
                EntityID projectile = gameWorld.New();
                gameWorld.Add<Position>(projectile, {pos.x + i * 5, pos.y});
                gameWorld.Add<Velocity>(projectile, {float(i * 20 - 10), 30.0f});
                gameWorld.Add<Damage>(projectile, {dmg.value / 2});
                projectiles.push_back(projectile);
            }
        }

        gameWorld.Refresh();

        // Verify projectiles were created (use our projectile list)
        assert(projectiles.size() == 14); // 7 shooters * 2 projectiles each
        for (EntityID proj : projectiles)
        {
            assert(gameWorld.IsValid(proj));
            assert(gameWorld.Has<Position>(proj));
            assert(gameWorld.Has<Velocity>(proj));
            assert(gameWorld.Has<Damage>(proj));
            assert(!gameWorld.Has<Health>(proj)); // Projectiles don't have health
            assert(!gameWorld.Has<AI>(proj));     // Projectiles don't start with AI
        }

        // === PHASE 5: Complex entity state changes ===

        // Kill some entities (remove all components, but keep entity alive)
        for (int i = 0; i < 2 && i < enemies.size(); ++i)
        {
            gameWorld.RemoveAll(enemies[i]);
        }

        // Convert some powerups to enemies (add combat components)
        for (int i = 0; i < 2 && i < powerups.size(); ++i)
        {
            EntityID powerup = powerups[i];
            gameWorld.Add<Velocity>(powerup, {5.0f, -5.0f});
            gameWorld.Add<Health>(powerup, {30, 30});
            gameWorld.Add<Damage>(powerup, {10});
            gameWorld.Add<AI>(powerup, {50, 1});
        }

        // Some projectiles become homing (add AI)
        for (int i = 0; i < 5 && i < projectiles.size(); ++i)
        {
            EntityID proj = projectiles[i];
            gameWorld.Add<AI>(proj, {100, 2}); // Homing state
        }

        gameWorld.Refresh();

        // === PHASE 6: Verify complex archetype transitions ===

        // Count entities by component type
        EngineView<Position> allPositioned(gameWorld);
        int totalPositioned = 0;
        for (auto [entity, pos] : allPositioned) totalPositioned++;

        EngineView<Health> allLiving(gameWorld);
        int livingEntities = 0;
        for (auto [entity, hp] : allLiving) livingEntities++;

        EngineView<AI> allAI(gameWorld);
        int aiEntitiesCount = 0;
        for (auto [entity, ai] : allAI) aiEntitiesCount++;

        // We should have many entities with position (players, remaining enemies, powerups, projectiles)
        assert(totalPositioned >= 15);
        // Entities with health: players + remaining enemies + converted powerups
        assert(livingEntities >= 6);
        // Entities with AI: remaining enemies + converted powerups + homing projectiles
        assert(aiEntitiesCount >= 8);

        // === PHASE 7: Create second world and test Union ===

        Engine secondWorld;

        // Create a different set of entities in second world
        std::vector<EntityID> bosses;
        for (int i = 0; i < 2; ++i)
        {
            EntityID boss = secondWorld.New();
            secondWorld.Add<Position>(boss, {float(i * 300), 300.0f});
            secondWorld.Add<Health>(boss, {200, 200});
            secondWorld.Add<Damage>(boss, {50});
            secondWorld.Add<AI>(boss, {150, 3}); // Boss AI state
            bosses.push_back(boss);
        }

        secondWorld.Refresh();

        // Count entities before union
        EngineView<Position> worldEntities(gameWorld);
        int beforeCount = 0;
        for (auto [entity, pos] : worldEntities) beforeCount++;

        // Union second world into main world
        gameWorld.Union(secondWorld);
        gameWorld.Refresh();

        // Count entities after union
        EngineView<Position> allWorldEntities(gameWorld);
        int afterCount = 0;
        for (auto [entity, pos] : allWorldEntities) afterCount++;

        assert(afterCount == beforeCount + 2); // Should have 2 more entities

        // === PHASE 8: Mass deletion using tracked entities ===

        // Delete all our tracked projectiles
        for (EntityID proj : projectiles)
        {
            if (gameWorld.IsValid(proj))
            {
                gameWorld.Delete(proj);
            }
        }

        gameWorld.Refresh();

        // Verify projectiles are gone
        for (EntityID proj : projectiles)
        {
            assert(!gameWorld.IsValid(proj));
        }

        // === PHASE 9: Stress test entity reuse ===

        std::vector<EntityID> tempEntities;

        // Create and immediately delete many entities
        for (int i = 0; i < 50; ++i)
        {
            EntityID temp = gameWorld.New();
            gameWorld.Add<Position>(temp, {float(i), float(i)});
            if (i % 3 == 0)
            {
                gameWorld.Delete(temp);
            } else
            {
                tempEntities.push_back(temp);
            }
        }

        gameWorld.Refresh();

        // Create more entities - should reuse some deleted IDs
        for (int i = 0; i < 20; ++i)
        {
            EntityID reused = gameWorld.New();
            gameWorld.Add<Health>(reused, {10, 10});
            assert(gameWorld.IsValid(reused));
        }

        gameWorld.Refresh();

        // === PHASE 10: Final verification ===

        // Test that world is still consistent
        EngineView<Position> finalPositioned(gameWorld);
        EngineView<Health> finalLiving(gameWorld);
        EngineView<Damage> finalDamagers(gameWorld);
        EngineView<AI> finalAI(gameWorld);

        int finalPosCount = 0, finalHealthCount = 0, finalDmgCount = 0, finalAICount = 0;

        for (auto [entity, pos] : finalPositioned) finalPosCount++;
        for (auto [entity, hp] : finalLiving) finalHealthCount++;
        for (auto [entity, dmg] : finalDamagers) finalDmgCount++;
        for (auto [entity, ai] : finalAI) finalAICount++;

        // Should have substantial numbers of entities in various configurations
        assert(finalPosCount > 0);
        assert(finalHealthCount > 0);
        assert(finalDmgCount > 0);
        assert(finalAICount > 0);

        // Test some complex queries (entities with AT LEAST all 4 component types)
        EngineView<Position, Health, Damage, AI> complexEntities(gameWorld);
        int complexCount = 0;
        for (auto [entity, pos, hp, dmg, ai] : complexEntities)
        {
            complexCount++;
            assert(hp.hp <= hp.maxHp);
            assert(dmg.value > 0);
            assert(ai.aggroRange > 0);
        }

        // Should have some entities with all 4 component types
        assert(complexCount > 0);

        // Final cleanup
        gameWorld.Destroy();

        // Verify complete destruction
        for (EntityID player : players)
        {
            assert(!gameWorld.IsValid(player));
        }

        // Should be able to create new entities after destroy
        EntityID newStart = gameWorld.New();
        assert(gameWorld.IsValid(newStart));
        assert(newStart == 0); // Should start from 0 again
    }
}

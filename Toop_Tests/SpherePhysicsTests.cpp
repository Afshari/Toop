#include "pch.h"
#include "SpherePhysics.h"
#include "TestHelpers.h"
using namespace Toop::TestHelpers;

namespace Toop {

    // --------------------------------------------------------------------------------
    // Init tests
    // --------------------------------------------------------------------------------

    TEST(SpherePhysicsTest, InitialPositionIsCorrect)
    {
        auto sphere = MakeSphere();
        EXPECT_FLOAT_EQ(sphere.GetPosX(), 0.0f);
        EXPECT_FLOAT_EQ(sphere.GetPosY(), 1.5f);
        EXPECT_FLOAT_EQ(sphere.GetPosZ(), 0.0f);
    }

    TEST(SpherePhysicsTest, InitialOrientationIsIdentity)
    {
        auto sphere = MakeSphere();
        EXPECT_FLOAT_EQ(sphere.GetQuatX(), 0.0f);
        EXPECT_FLOAT_EQ(sphere.GetQuatY(), 0.0f);
        EXPECT_FLOAT_EQ(sphere.GetQuatZ(), 0.0f);
        EXPECT_FLOAT_EQ(sphere.GetQuatW(), 1.0f);
    }

    TEST(SpherePhysicsTest, InitialIdleIsFalse)
    {
        auto sphere = MakeSphere();
        EXPECT_FALSE(sphere.IsIdle());
    }

    // --------------------------------------------------------------------------------
    // Gravity tests
    // --------------------------------------------------------------------------------

    TEST(SpherePhysicsTest, GravityMovesSpherDown)
    {
        auto sphere = MakeSphere();
        float initial_y = sphere.GetPosY();
        sphere.Update(1.0f / 60.0f);
        EXPECT_LT(sphere.GetPosY(), initial_y);
    }

    TEST(SpherePhysicsTest, SphereDoesNotFallThroughGround)
    {
        auto sphere = MakeSphere();
        auto config = MakeDefaultConfig();
        float floor_y = config.room.ground_y + config.sim.sphere_radius;

        for (int i = 0; i < 300; i++)
            sphere.Update(1.0f / 60.0f);

        EXPECT_GE(sphere.GetPosY(), floor_y - 1e-4f);
    }

    // --------------------------------------------------------------------------------
    // Bounce tests
    // --------------------------------------------------------------------------------

    TEST(SpherePhysicsTest, SphereBounceRevertsVelocity)
    {
        auto config = MakeDefaultConfig();
        SpherePhysics sphere;

        // place sphere just above ground so it bounces immediately
        config.sphere.gravity = -9.8f;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);

        // run until it hits ground
        for (int i = 0; i < 120; i++)
            sphere.Update(1.0f / 60.0f);

        // after bounce sphere should be moving upward or at rest
        float floor_y = config.room.ground_y + config.sim.sphere_radius;
        EXPECT_GE(sphere.GetPosY(), floor_y - 1e-4f);
    }

    TEST(SpherePhysicsTest, SphereStaysWithinRoomWalls)
    {
        auto config = MakeDefaultConfig();
        SpherePhysics sphere;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);

        // give it a strong horizontal velocity
        sphere.StartDrag(2.4f, 1.5f, 0.0f);
        sphere.EndDrag();

        for (int i = 0; i < 300; i++)
            sphere.Update(1.0f / 60.0f);

        float r = config.sim.sphere_radius;
        EXPECT_GE(sphere.GetPosX(), config.room.min.x + r - 1e-4f);
        EXPECT_LE(sphere.GetPosX(), config.room.max.x - r + 1e-4f);
        EXPECT_GE(sphere.GetPosZ(), config.room.min.z + r - 1e-4f);
        EXPECT_LE(sphere.GetPosZ(), config.room.max.z - r + 1e-4f);
    }

    // --------------------------------------------------------------------------------
    // Drag tests
    // --------------------------------------------------------------------------------

    TEST(SpherePhysicsTest, DragMovesSphereToTarget)
    {
        auto sphere = MakeSphere();
        sphere.StartDrag(1.0f, 1.5f, 0.0f);

        for (int i = 0; i < 60; i++)
            sphere.Update(1.0f / 60.0f);

        EXPECT_NEAR(sphere.GetPosX(), 1.0f, 0.01f);
        EXPECT_NEAR(sphere.GetPosY(), 1.5f, 0.01f);
    }

    TEST(SpherePhysicsTest, EndDragAppliesThrowVelocity)
    {
        auto sphere = MakeSphere();
        sphere.StartDrag(0.0f, 1.5f, 0.0f);
        sphere.UpdateDrag(1.0f, 1.5f, 0.0f);

        for (int i = 0; i < 10; i++)
            sphere.Update(1.0f / 60.0f);

        sphere.EndDrag();
        sphere.Update(1.0f / 60.0f);

        // after throw sphere should have non-zero velocity - delta should be non-zero
        float delta_len = sphere.GetDeltaX() * sphere.GetDeltaX()
            + sphere.GetDeltaY() * sphere.GetDeltaY()
            + sphere.GetDeltaZ() * sphere.GetDeltaZ();
        EXPECT_GT(delta_len, 0.0f);
    }

    // --------------------------------------------------------------------------------
    // Idle detection tests
    // --------------------------------------------------------------------------------

    TEST(SpherePhysicsTest, IdleDetectedAfterThreshold)
    {
        auto config = MakeDefaultConfig();
        SpherePhysics sphere;

        // zero gravity so sphere stays still
        config.sphere.gravity = 0.0f;
        config.sphere.idle_threshold_sec = 0.5f;
        config.sphere.idle_speed_threshold = 0.05f;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);

        // run for longer than idle threshold
        int frames = static_cast<int>(0.6f / (1.0f / 60.0f));
        for (int i = 0; i < frames; i++)
            sphere.Update(1.0f / 60.0f);

        EXPECT_TRUE(sphere.IsIdle());
    }

    TEST(SpherePhysicsTest, IdleResetOnMovement)
    {
        auto config = MakeDefaultConfig();
        SpherePhysics sphere;

        config.sphere.gravity = 0.0f;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);

        // become idle first
        int frames = static_cast<int>(0.6f / (1.0f / 60.0f));
        for (int i = 0; i < frames; i++)
            sphere.Update(1.0f / 60.0f);

        EXPECT_TRUE(sphere.IsIdle());

        // now drag it
        sphere.StartDrag(1.0f, 1.5f, 0.0f);
        sphere.UpdateDrag(1.0f, 1.5f, 0.0f);
        sphere.Update(1.0f / 60.0f);
        sphere.EndDrag();

        // give it velocity and update
        for (int i = 0; i < 5; i++)
            sphere.Update(1.0f / 60.0f);

        EXPECT_FALSE(sphere.IsIdle());
    }

    // --------------------------------------------------------------------------------
    // Orientation tests
    // --------------------------------------------------------------------------------

    TEST(SpherePhysicsTest, OrientationQuatRemainsNormalized)
    {
        auto sphere = MakeSphere();

        for (int i = 0; i < 300; i++)
            sphere.Update(1.0f / 60.0f);

        float len = sqrtf(
            sphere.GetQuatX() * sphere.GetQuatX() +
            sphere.GetQuatY() * sphere.GetQuatY() +
            sphere.GetQuatZ() * sphere.GetQuatZ() +
            sphere.GetQuatW() * sphere.GetQuatW());

        EXPECT_NEAR(len, 1.0f, 1e-5f);
    }

    TEST(SpherePhysicsTest, OrientationChangesWhenMoving)
    {
        auto sphere = MakeSphere();

        // drag to give it horizontal velocity so rolling axis is valid
        sphere.StartDrag(1.0f, 1.5f, 0.0f);
        for (int i = 0; i < 20; i++)
            sphere.Update(1.0f / 60.0f);
        sphere.EndDrag();

        // let it move freely
        for (int i = 0; i < 60; i++)
            sphere.Update(1.0f / 60.0f);

        float len = sqrtf(
            sphere.GetQuatX() * sphere.GetQuatX() +
            sphere.GetQuatY() * sphere.GetQuatY() +
            sphere.GetQuatZ() * sphere.GetQuatZ());

        // xyz components should be non-zero when rolling horizontally
        EXPECT_GT(len, 1e-4f);
    }

} // namespace Toop
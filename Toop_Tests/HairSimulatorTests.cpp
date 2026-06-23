#include "pch.h"
#include "cpu/HairSimulator.h"
#include "Config.h"
#include "TestHelpers.h"

namespace Toop {

    // --------------------------------------------------------------------------------
    // Helpers
    // --------------------------------------------------------------------------------

    static SimConfig MakeSmallSimConfig()
    {
        SimConfig sim = Config::Default().sim;
        sim.num_strands = 64;
        sim.num_segments = 3;
        sim.num_substeps = 2;
        return sim;
    }

    static BaldPatchConfig MakeBaldPatches()
    {
        return Config::Default().bald_patches;
    }

    // --------------------------------------------------------------------------------
    // Init / shutdown
    // --------------------------------------------------------------------------------

    TEST(HairSimulatorTest, InitializesSuccessfully)
    {
        HairSimulator sim;
        EXPECT_NO_THROW(sim.Init(MakeSmallSimConfig(), MakeBaldPatches()));
        EXPECT_TRUE(sim.IsInitialized());
        sim.Shutdown();
    }

    TEST(HairSimulatorTest, TotalParticlesIsCorrect)
    {
        HairSimulator sim;
        SimConfig cfg = MakeSmallSimConfig();
        sim.Init(cfg, MakeBaldPatches());

        int expected = cfg.num_strands * (cfg.num_segments + 1);
        EXPECT_EQ(sim.GetTotalParticles(), expected);

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, ShutdownSetsInitializedFalse)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());
        EXPECT_TRUE(sim.IsInitialized());

        sim.Shutdown();
        EXPECT_FALSE(sim.IsInitialized());
    }

    TEST(HairSimulatorTest, DoubleShutdownDoesNotCrash)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());
        sim.Shutdown();
        EXPECT_NO_THROW(sim.Shutdown());
    }

    // --------------------------------------------------------------------------------
    // Step
    // --------------------------------------------------------------------------------

    TEST(HairSimulatorTest, SingleStepDoesNotThrow)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, StepReturnsNonZeroTotalTime)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        StepTimings t = sim.Step(1.0f / 60.0f);
        EXPECT_GT(t.total_ms, 0.0f);

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, StepReturnsNonZeroPerKernelTimings)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        StepTimings t = sim.Step(1.0f / 60.0f);
        EXPECT_GT(t.update_roots_ms, 0.0f);
        EXPECT_GT(t.integrate_ms, 0.0f);
        EXPECT_GT(t.constraints_ms, 0.0f);
        EXPECT_GT(t.sphere_collision_ms, 0.0f);
        EXPECT_GT(t.ground_collision_ms, 0.0f);

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, MultipleStepsStable)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        for (int i = 0; i < 60; i++)
            EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, StepBeforeInitReturnZeroTimings)
    {
        HairSimulator sim;
        StepTimings t = sim.Step(1.0f / 60.0f);

        EXPECT_FLOAT_EQ(t.total_ms, 0.0f);
        EXPECT_FLOAT_EQ(t.update_roots_ms, 0.0f);
        EXPECT_FLOAT_EQ(t.integrate_ms, 0.0f);
        EXPECT_FLOAT_EQ(t.constraints_ms, 0.0f);
        EXPECT_FLOAT_EQ(t.sphere_collision_ms, 0.0f);
        EXPECT_FLOAT_EQ(t.ground_collision_ms, 0.0f);
    }

    // --------------------------------------------------------------------------------
    // Sphere state
    // --------------------------------------------------------------------------------

    TEST(HairSimulatorTest, SetSphereStateDoesNotThrow)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        EXPECT_NO_THROW(sim.SetSphereState(
            0.0f, 1.5f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f));

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, SetSphereStateThenStepDoesNotThrow)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        sim.SetSphereState(0.1f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));

        sim.Shutdown();
    }

    // --------------------------------------------------------------------------------
    // Wind
    // --------------------------------------------------------------------------------

    TEST(HairSimulatorTest, SetWindThenStepDoesNotThrow)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        sim.SetWind(0.5f, 0.0f, 0.3f);
        EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));

        sim.Shutdown();
    }

    TEST(HairSimulatorTest, SetWindZeroThenStepDoesNotThrow)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        sim.SetWind(0.0f, 0.0f, 0.0f);
        EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));

        sim.Shutdown();
    }

    // --------------------------------------------------------------------------------
    // Config params
    // --------------------------------------------------------------------------------

    TEST(HairSimulatorTest, ThreadsPerBlockFromConfig)
    {
        HairSimulator sim;
        SimConfig cfg = MakeSmallSimConfig();
        cfg.threads_per_block = 64;
        EXPECT_NO_THROW(sim.Init(cfg, MakeBaldPatches()));
        EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));
        sim.Shutdown();
    }

    TEST(HairSimulatorTest, LargeThreadsPerBlockDoesNotCrash)
    {
        HairSimulator sim;
        SimConfig cfg = MakeSmallSimConfig();
        cfg.threads_per_block = 512;
        EXPECT_NO_THROW(sim.Init(cfg, MakeBaldPatches()));
        EXPECT_NO_THROW(sim.Step(1.0f / 60.0f));
        sim.Shutdown();
    }

    // --------------------------------------------------------------------------------
    // Regression - run N frames, total time must be plausible
    // --------------------------------------------------------------------------------

    TEST(HairSimulatorTest, RegressionFrameTimeIsPlausible)
    {
        HairSimulator sim;
        sim.Init(MakeSmallSimConfig(), MakeBaldPatches());

        float total = 0.0f;
        int   frames = 30;

        for (int i = 0; i < frames; i++)
        {
            sim.SetSphereState(
                0.0f, 1.5f + i * 0.001f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
            StepTimings t = sim.Step(1.0f / 60.0f);
            total += t.total_ms;
        }

        float avg = total / frames;

        // avg frame time must be positive and under 100ms for a 64-strand sim
        EXPECT_GT(avg, 0.0f);
        EXPECT_LT(avg, 100.0f);

        sim.Shutdown();
    }

} // namespace Toop
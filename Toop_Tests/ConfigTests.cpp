#include "pch.h"
#include "Config.h"
#include <filesystem>
#include <fstream>

namespace Toop {

    // --------------------------------------------------------------------------------
    // Default tests
    // --------------------------------------------------------------------------------

    TEST(ConfigTest, DefaultReturnsValidConfig)
    {
        auto config = Config::Default();
        EXPECT_GT(config.sim.num_strands, 0);
        EXPECT_GT(config.sim.num_segments, 0);
        EXPECT_GT(config.sim.sphere_radius, 0.0f);
        EXPECT_GT(config.sim.num_substeps, 0);
    }

    TEST(ConfigTest, DefaultSimParamsAreCorrect)
    {
        auto config = Config::Default();
        EXPECT_EQ(config.sim.num_strands, 14549);
        EXPECT_EQ(config.sim.num_segments, 5);
        EXPECT_FLOAT_EQ(config.sim.segment_length, 0.035f);
        EXPECT_FLOAT_EQ(config.sim.gravity, -1.0f);
        EXPECT_EQ(config.sim.num_substeps, 15);
        EXPECT_FLOAT_EQ(config.sim.damping, 0.995f);
        EXPECT_FLOAT_EQ(config.sim.compliance, 0.001f);
        EXPECT_FLOAT_EQ(config.sim.sphere_radius, 0.4f);
    }

    TEST(ConfigTest, DefaultSphereParamsAreCorrect)
    {
        auto config = Config::Default();
        EXPECT_FLOAT_EQ(config.sphere.gravity, -1.0f);
        EXPECT_FLOAT_EQ(config.sphere.restitution, 0.95f);
        EXPECT_FLOAT_EQ(config.sphere.damping, 0.99f);
        EXPECT_FLOAT_EQ(config.sphere.drag_smoothing, 0.5f);
        EXPECT_FLOAT_EQ(config.sphere.throw_multiplier, 0.3f);
        EXPECT_FLOAT_EQ(config.sphere.idle_threshold_sec, 0.5f);
        EXPECT_FLOAT_EQ(config.sphere.idle_speed_threshold, 0.05f);
    }

    TEST(ConfigTest, DefaultRoomParamsAreCorrect)
    {
        auto config = Config::Default();
        EXPECT_FLOAT_EQ(config.room.ground_y, 0.0f);
        EXPECT_FLOAT_EQ(config.room.min.x, -2.5f);
        EXPECT_FLOAT_EQ(config.room.min.y, 0.0f);
        EXPECT_FLOAT_EQ(config.room.min.z, -2.5f);
        EXPECT_FLOAT_EQ(config.room.max.x, 2.5f);
        EXPECT_FLOAT_EQ(config.room.max.y, 3.0f);
        EXPECT_FLOAT_EQ(config.room.max.z, 2.5f);
    }

    TEST(ConfigTest, DefaultBaldPatchParamsAreCorrect)
    {
        auto config = Config::Default();
        EXPECT_FLOAT_EQ(config.bald_patches.eye_threshold, 0.97f);
        EXPECT_FLOAT_EQ(config.bald_patches.top_threshold, 0.95f);
        EXPECT_FLOAT_EQ(config.bald_patches.eye_left_dir.x, -0.25f);
        EXPECT_FLOAT_EQ(config.bald_patches.eye_left_dir.y, 0.15f);
        EXPECT_FLOAT_EQ(config.bald_patches.eye_left_dir.z, 1.0f);
        EXPECT_FLOAT_EQ(config.bald_patches.eye_right_dir.x, 0.25f);
        EXPECT_FLOAT_EQ(config.bald_patches.eye_right_dir.y, 0.15f);
        EXPECT_FLOAT_EQ(config.bald_patches.eye_right_dir.z, 1.0f);
    }

    TEST(ConfigTest, DefaultProfileParamsAreCorrect)
    {
        auto config = Config::Default();
        EXPECT_FALSE(config.profile.enabled);
        EXPECT_EQ(config.profile.warmup_frames, 60);
        EXPECT_EQ(config.profile.capture_frames, 300);
        EXPECT_EQ(config.profile.output_dir, "output");
    }

    TEST(ConfigTest, DefaultRenderParamsAreCorrect)
    {
        auto config = Config::Default();
        EXPECT_EQ(config.render.window_width, 1280);
        EXPECT_EQ(config.render.window_height, 720);
        EXPECT_TRUE(config.render.vsync);
    }

    // --------------------------------------------------------------------------------
    // Load tests
    // --------------------------------------------------------------------------------

    TEST(ConfigTest, LoadThrowsOnMissingFile)
    {
        EXPECT_THROW(
            Config::Load("nonexistent_path/config.json"),
            std::runtime_error);
    }

    TEST(ConfigTest, LoadParsesValidFile)
    {
        // write a minimal valid config.json to a temp file
        auto temp_path = std::filesystem::temp_directory_path() / "toop_test_config.json";

        std::ofstream f(temp_path);
        f << R"({
            "sim": {
                "num_strands": 100,
                "num_segments": 5,
                "segment_length": 0.035,
                "gravity": -1.0,
                "num_substeps": 15,
                "damping": 0.995,
                "compliance": 0.001,
                "sphere_radius": 0.4,
                "sphere_center": { "x": 0.0, "y": 1.5, "z": 0.0 },
                "wind_strength": 0.15,
                "wind_frequency": 0.5
            },
            "sphere": {
                "gravity": -1.0,
                "restitution": 0.95,
                "damping": 0.99,
                "drag_smoothing": 0.5,
                "throw_multiplier": 0.3,
                "idle_threshold_sec": 0.5,
                "idle_speed_threshold": 0.05
            },
            "room": {
                "min": { "x": -2.5, "y": 0.0, "z": -2.5 },
                "max": { "x":  2.5, "y": 3.0, "z":  2.5 },
                "ground_y": 0.0
            },
            "bald_patches": {
                "eye_left_dir":  { "x": -0.25, "y": 0.15, "z": 1.0 },
                "eye_right_dir": { "x":  0.25, "y": 0.15, "z": 1.0 },
                "eye_threshold": 0.97,
                "top_threshold": 0.95
            },
            "render": {
                "window_width": 1280,
                "window_height": 720,
                "vsync": true
            },
            "profile": {
                "enabled": false,
                "warmup_frames": 60,
                "capture_frames": 300,
                "output_dir": "output"
            }
        })";
        f.close();

        auto config = Config::Load(temp_path.string());
        EXPECT_EQ(config.sim.num_strands, 100);
        EXPECT_EQ(config.sim.num_segments, 5);
        EXPECT_FLOAT_EQ(config.sim.sphere_radius, 0.4f);
        EXPECT_FLOAT_EQ(config.sphere.restitution, 0.95f);
        EXPECT_FLOAT_EQ(config.room.ground_y, 0.0f);

        std::filesystem::remove(temp_path);
    }

    TEST(ConfigTest, LoadThrowsOnInvalidJson)
    {
        auto temp_path = std::filesystem::temp_directory_path() / "toop_bad_config.json";

        std::ofstream f(temp_path);
        f << "{ this is not valid json }";
        f.close();

        EXPECT_THROW(
            Config::Load(temp_path.string()),
            std::exception);

        std::filesystem::remove(temp_path);
    }

} // namespace Toop
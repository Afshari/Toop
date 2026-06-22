#pragma once
#include <string>
#include <cstdint>
#include <filesystem>

namespace Toop {

    struct Vec3Config
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct SimConfig
    {
        int         num_strands = 14549;
        int         num_segments = 5;
        float       segment_length = 0.035f;
        float       gravity = -1.0f;
        int         num_substeps = 15;
        float       damping = 0.995f;
        float       compliance = 0.001f;
        float       sphere_radius = 0.4f;
        Vec3Config  sphere_center = { 0.0f, 1.5f, 0.0f };
        float       wind_strength = 0.15f;
        float       wind_frequency = 0.5f;
    };

    struct SphereConfig
    {
        float gravity = -1.0f;
        float restitution = 0.95f;
        float damping = 0.99f;
        float drag_smoothing = 0.5f;
        float throw_multiplier = 0.3f;
        float idle_threshold_sec = 0.5f;
        float idle_speed_threshold = 0.05f;
        float idle_rotate_speed = 0.04f;
    };

    struct RoomConfig
    {
        Vec3Config  min = { -2.5f, 0.0f, -2.5f };
        Vec3Config  max = { 2.5f, 3.0f,  2.5f };
        float       ground_y = 0.0f;
    };

    struct BaldPatchConfig
    {
        Vec3Config  eye_left_dir = { -0.25f, 0.15f, 1.0f };
        Vec3Config  eye_right_dir = { 0.25f, 0.15f, 1.0f };
        float       eye_threshold = 0.97f;
        float       top_threshold = 1.0f;
    };

    struct RenderConfig
    {
        int  window_width = 1280;
        int  window_height = 720;
        bool vsync = true;
    };

    struct ProfileConfig
    {
        bool        enabled = false;
        int         warmup_frames = 60;
        int         capture_frames = 300;
        std::string output_dir = "output";
    };

    struct Config
    {
        SimConfig       sim;
        SphereConfig    sphere;
        RoomConfig      room;
        BaldPatchConfig bald_patches;
        RenderConfig    render;
        ProfileConfig   profile;

        // Throws std::runtime_error if file not found or invalid
        static Config Load(const std::string& filepath);

        // Safe fallback defaults
        static Config Default();
    };

} // namespace Toop
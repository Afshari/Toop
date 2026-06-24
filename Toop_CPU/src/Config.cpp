#include "Config.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdexcept>

namespace Toop {

    // --------------------------------------------------------------------------------
    static Vec3Config LoadVec3(const boost::property_tree::ptree& node)
    {
        Vec3Config v;
        v.x = node.get<float>("x");
        v.y = node.get<float>("y");
        v.z = node.get<float>("z");
        return v;
    }

    // --------------------------------------------------------------------------------
    Config Config::Load(const std::string& filepath)
    {
        if (!std::filesystem::exists(filepath))
            throw std::runtime_error("[ERROR] Config file not found: " + filepath);

        boost::property_tree::ptree pt;
        boost::property_tree::read_json(filepath, pt);

        Config cfg;

        // sim
        cfg.sim.num_strands = pt.get<int>("sim.num_strands");
        cfg.sim.num_segments = pt.get<int>("sim.num_segments");
        cfg.sim.segment_length = pt.get<float>("sim.segment_length");
        cfg.sim.gravity = pt.get<float>("sim.gravity");
        cfg.sim.num_substeps = pt.get<int>("sim.num_substeps");
        cfg.sim.damping = pt.get<float>("sim.damping");
        cfg.sim.compliance = pt.get<float>("sim.compliance");
        cfg.sim.sphere_radius = pt.get<float>("sim.sphere_radius");
        cfg.sim.sphere_center = LoadVec3(pt.get_child("sim.sphere_center"));
        cfg.sim.wind_strength = pt.get<float>("sim.wind_strength");
        cfg.sim.wind_frequency = pt.get<float>("sim.wind_frequency");
        cfg.sim.threads_per_block = pt.get<int>("sim.threads_per_block");

        // sphere
        cfg.sphere.gravity = pt.get<float>("sphere.gravity");
        cfg.sphere.restitution = pt.get<float>("sphere.restitution");
        cfg.sphere.damping = pt.get<float>("sphere.damping");
        cfg.sphere.drag_smoothing = pt.get<float>("sphere.drag_smoothing");
        cfg.sphere.throw_multiplier = pt.get<float>("sphere.throw_multiplier");
        cfg.sphere.idle_threshold_sec = pt.get<float>("sphere.idle_threshold_sec");
        cfg.sphere.idle_speed_threshold = pt.get<float>("sphere.idle_speed_threshold");

        // room
        cfg.room.min = LoadVec3(pt.get_child("room.min"));
        cfg.room.max = LoadVec3(pt.get_child("room.max"));
        cfg.room.ground_y = pt.get<float>("room.ground_y");

        // bald patches
        cfg.bald_patches.eye_left_dir = LoadVec3(pt.get_child("bald_patches.eye_left_dir"));
        cfg.bald_patches.eye_right_dir = LoadVec3(pt.get_child("bald_patches.eye_right_dir"));
        cfg.bald_patches.eye_threshold = pt.get<float>("bald_patches.eye_threshold");
        cfg.bald_patches.top_threshold = pt.get<float>("bald_patches.top_threshold");

        // render
        cfg.render.window_width = pt.get<int>("render.window_width");
        cfg.render.window_height = pt.get<int>("render.window_height");
        cfg.render.vsync = pt.get<bool>("render.vsync");

        // profile
        cfg.profile.enabled = pt.get<bool>("profile.enabled");
        cfg.profile.warmup_frames = pt.get<int>("profile.warmup_frames");
        cfg.profile.capture_frames = pt.get<int>("profile.capture_frames");
        cfg.profile.output_dir = pt.get<std::string>("profile.output_dir");

        return cfg;
    }

    // --------------------------------------------------------------------------------
    Config Config::Default()
    {
        return Config{};
    }

} // namespace Toop
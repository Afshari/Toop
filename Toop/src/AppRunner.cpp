#include "AppRunner.h"
#include "cpu/HairSimulator.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>

namespace Toop {

    // --------------------------------------------------------------------------------
    static void WriteTimingsCsv(
        const std::string& outputDir,
        const std::vector<StepTimings>& timings)
    {
        std::filesystem::create_directories(outputDir);
        auto csvPath = std::filesystem::path(outputDir) / "headless_timings.csv";

        std::ofstream csv(csvPath);
        csv << "frame,sphere_collision_ms,ground_collision_ms,total_ms\n";

        for (size_t i = 0; i < timings.size(); i++)
        {
            csv << i << ","
                << std::fixed << std::setprecision(4)
                << timings[i].sphere_collision_ms << ","
                << timings[i].ground_collision_ms << ","
                << timings[i].total_ms << "\n";
        }

        std::cout << "[INFO] Timings written to: " << csvPath << std::endl;
    }

    // --------------------------------------------------------------------------------
    int AppRunner::Run(const Config& config)
    {
        std::cout << "[INFO] Starting Toop interactive mode." << std::endl;
        std::cout << "[INFO] Strands: " << config.sim.num_strands << std::endl;
        std::cout << "[INFO] Segments: " << config.sim.num_segments << std::endl;
        std::cout << "[INFO] Substeps: " << config.sim.num_substeps << std::endl;
        std::cout << "[INFO] Window: " << config.render.window_width
            << "x" << config.render.window_height << std::endl;

        // TODO: initialize OpenGL window, simulation, main loop

        return 0;
    }

    // --------------------------------------------------------------------------------
    int AppRunner::RunHeadless(const Config& config)
    {
        std::cout << "[INFO] Starting Toop headless mode." << std::endl;
        std::cout << "[INFO] Strands: " << config.sim.num_strands << std::endl;
        std::cout << "[INFO] Warmup frames: " << config.profile.warmup_frames << std::endl;
        std::cout << "[INFO] Capture frames: " << config.profile.capture_frames << std::endl;
        std::cout << "[INFO] Output dir: " << config.profile.output_dir << std::endl;

        HairSimulator sim;
        sim.Init(config.sim, config.bald_patches);

        // warmup - not measured
        std::cout << "[INFO] Running warmup..." << std::endl;
        for (int i = 0; i < config.profile.warmup_frames; i++)
            sim.Step(1.0f / 60.0f);

        // capture - measured
        std::cout << "[INFO] Running capture..." << std::endl;
        std::vector<StepTimings> timings;
        timings.reserve(config.profile.capture_frames);

        for (int i = 0; i < config.profile.capture_frames; i++)
            timings.push_back(sim.Step(1.0f / 60.0f));

        // summary
        float avg_total = 0.0f;
        for (const auto& t : timings)
            avg_total += t.total_ms;
        avg_total /= timings.size();

        std::cout << "[INFO] Avg frame time: "
            << std::fixed << std::setprecision(3)
            << avg_total << " ms" << std::endl;
        std::cout << "[INFO] Avg FPS: "
            << std::setprecision(1)
            << (1000.0f / avg_total) << std::endl;

        WriteTimingsCsv(config.profile.output_dir, timings);

        sim.Shutdown();
        return 0;
    }

    // --------------------------------------------------------------------------------
    int AppRunner::RunBench(const Config& config)
    {
        std::cout << "[INFO] Starting Toop bench mode." << std::endl;
        std::cout << "[INFO] Strands: " << config.sim.num_strands << std::endl;
        std::cout << "[INFO] Warmup frames: " << config.profile.warmup_frames << std::endl;
        std::cout << "[INFO] Capture frames: " << config.profile.capture_frames << std::endl;
        std::cout << "[INFO] Output dir: " << config.profile.output_dir << std::endl;

        // TODO: run fixed frames, measure per-frame timing, write CSV

        return 0;
    }

} // namespace Toop
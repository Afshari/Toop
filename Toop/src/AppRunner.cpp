#include "AppRunner.h"
#include <iostream>

namespace Toop {

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
        std::cout << "[INFO] Starting Toop headless mode (no OpenGL)." << std::endl;
        std::cout << "[INFO] Strands: " << config.sim.num_strands << std::endl;
        std::cout << "[INFO] Warmup frames: " << config.profile.warmup_frames << std::endl;
        std::cout << "[INFO] Capture frames: " << config.profile.capture_frames << std::endl;
        std::cout << "[INFO] Output dir: " << config.profile.output_dir << std::endl;

        // TODO: initialize simulation, run fixed frames, write profiling CSV

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
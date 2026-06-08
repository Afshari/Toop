#include "AppRunner.h"
#include "cpu/HairSimulator.h"
#include "cpu/Renderer.h"
#include "SpherePhysics.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>

#ifndef TOOP_HEADLESS
#include "cpu/Window.h"
#include "Camera.h"
#include "cpu/HairSimulator.h"
#include "SpherePhysics.h"
#include <chrono>
#endif


namespace Toop {

    // --------------------------------------------------------------------------------
    static void WriteTimingsCsv(
        const std::string& outputDir,
        const std::vector<StepTimings>& timings)
    {
        std::filesystem::create_directories(outputDir);
        auto csvPath = std::filesystem::path(outputDir) / "headless_timings.csv";

        std::ofstream csv(csvPath);
        csv << "frame,sphere_collision_ms,ground_collision_ms,integrate_ms,update_roots_ms,constraints_ms,total_ms\n";

        for (size_t i = 0; i < timings.size(); i++)
        {
            csv << i << ","
                << std::fixed << std::setprecision(4)
                << timings[i].sphere_collision_ms << ","
                << timings[i].ground_collision_ms << ","
                << timings[i].integrate_ms << ","
                << timings[i].update_roots_ms << ","
                << timings[i].constraints_ms << ","
                << timings[i].total_ms << "\n";
        }

        std::cout << "[INFO] Timings written to: " << csvPath << std::endl;
    }

    // --------------------------------------------------------------------------------
    int AppRunner::Run(const Config& config)
    {
#ifdef TOOP_HEADLESS
        std::cout << "[WARN] Built headless - redirecting to RunHeadless." << std::endl;
        return RunHeadless(config);
#else
        std::cout << "[INFO] Starting Toop." << std::endl;

        Window window("Toop", config.render.window_width,
            config.render.window_height,
            config.render.vsync);

        Camera camera;
        camera.SetPreset(2, glm::vec3(
            config.sim.sphere_center.x,
            config.sim.sphere_center.y,
            config.sim.sphere_center.z));

        SpherePhysics sphere;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);

        HairSimulator sim;
        sim.Init(config.sim, config.bald_patches);

        Renderer renderer;
        renderer.Init(config, sim);

        // key callback
        window.SetKeyCallback([&](int key, int action)
            {
                if (action == GLFW_PRESS || action == GLFW_REPEAT)
                {
                    switch (key)
                    {
                    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(
                        glfwGetCurrentContext(), true); break;
                    case GLFW_KEY_W:     camera.HandleKeyW();    break;
                    case GLFW_KEY_S:     camera.HandleKeyS();    break;
                    case GLFW_KEY_A:     camera.HandleKeyA();    break;
                    case GLFW_KEY_D:     camera.HandleKeyD();    break;
                    case GLFW_KEY_Q:     camera.HandleKeyQ();    break;
                    case GLFW_KEY_E:     camera.HandleKeyE();    break;
                    case GLFW_KEY_1:     camera.SetPreset(1, glm::vec3(
                        sphere.GetPosX(),
                        sphere.GetPosY(),
                        sphere.GetPosZ())); break;
                    case GLFW_KEY_2:     camera.SetPreset(2, glm::vec3(
                        sphere.GetPosX(),
                        sphere.GetPosY(),
                        sphere.GetPosZ())); break;
                    case GLFW_KEY_3:     camera.SetPreset(3, glm::vec3(
                        sphere.GetPosX(),
                        sphere.GetPosY(),
                        sphere.GetPosZ())); break;
                    }
                }
            });

        // mouse callback
        window.SetMouseCallback([&](const MouseState& mouse)
            {
                if (mouse.left)
                    camera.HandleMouseOrbit(mouse.dx, mouse.dy, glm::vec3(
                        sphere.GetPosX(),
                        sphere.GetPosY(),
                        sphere.GetPosZ()));
                else if (mouse.right)
                    camera.HandleMouseTranslate(mouse.dx, mouse.dy);
            });

        // scroll callback
        window.SetScrollCallback([&](float delta)
            {
                camera.HandleWheel(delta);
            });

        // resize callback
        window.SetResizeCallback([&](int /*width*/, int /*height*/)
            {
                // camera aspect updated via window.GetAspect() each frame
            });

        // main loop
        auto prev_time = std::chrono::high_resolution_clock::now();

        while (!window.ShouldClose())
        {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - prev_time).count();
            prev_time = now;
            dt = std::min(dt, 0.05f); // clamp to avoid spiral of death

            window.BeginFrame();

            sphere.Update(dt);
            sim.SetSphereState(
                sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ(),
                sphere.GetQuatX(), sphere.GetQuatY(), sphere.GetQuatZ(),
                sphere.GetQuatW());
            sim.Step(dt);

            renderer.MapInterop();
            sim.SetInteropBuffer(renderer.GetInteropPtr());
            sim.PackPositionsForRendering();
            renderer.UnmapInterop();

            renderer.Render(
                camera.GetViewMatrix(),
                camera.GetProjectionMatrix(window.GetAspect()),
                glm::vec3(sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ()),
                config.sim.sphere_radius);

            window.EndFrame();
        }

        renderer.Shutdown();
        sim.Shutdown();
        return 0;
#endif
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

        SpherePhysics sphere;
        sphere.Init(config.sphere, config.room, config.sim.sphere_radius);

        // warmup - not measured
        std::cout << "[INFO] Running warmup..." << std::endl;
        for (int i = 0; i < config.profile.warmup_frames; i++)
        {
            sphere.Update(1.0f / 60.0f);
            sim.SetSphereState(
                sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ(),
                sphere.GetQuatX(), sphere.GetQuatY(), sphere.GetQuatZ(), sphere.GetQuatW());
            sim.Step(1.0f / 60.0f);
        }

        // capture - measured
        std::cout << "[INFO] Running capture..." << std::endl;
        std::vector<StepTimings> timings;
        timings.reserve(config.profile.capture_frames);

        for (int i = 0; i < config.profile.capture_frames; i++)
        {
            sphere.Update(1.0f / 60.0f);
            sim.SetSphereState(
                sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ(),
                sphere.GetQuatX(), sphere.GetQuatY(), sphere.GetQuatZ(), sphere.GetQuatW());
            timings.push_back(sim.Step(1.0f / 60.0f));
        }

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
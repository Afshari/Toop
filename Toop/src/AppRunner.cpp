#include "AppRunner.h"
#include "cpu/HairSimulator.h"
#include "SpherePhysics.h"
#include "RayUtils.h"
#include "InputHandler.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>

#ifndef TOOP_HEADLESS
#include "cpu/Renderer.h"
#include "cpu/Window.h"
#include "Camera.h"
#include "cpu/HairSimulator.h"
#include "SpherePhysics.h"
#include <chrono>
#include <glm/gtc/quaternion.hpp>
#endif

#ifdef TOOP_DEBUG
#include "DebugUI.h"
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

        // --- systems ---
        Window window("Toop",
            config.render.window_width,
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
        renderer.MapInterop();
        sim.SetInteropBuffer(renderer.GetInteropPtr());
        renderer.UnmapInterop();

        InputHandler input;
        input.Init(&window, &camera, &sphere, &config);
        input.RegisterCallbacks();

#ifdef TOOP_DEBUG
        DebugUI      debug_ui;
        DebugUIState debug_state;
        debug_ui.Init(window.GetGLFWWindow());
        debug_state.compliance = config.sim.compliance;
        debug_state.damping = config.sim.damping;
        debug_state.wind_strength = config.sim.wind_strength;
        debug_state.wind_frequency = config.sim.wind_frequency;
        debug_state.gravity = config.sim.gravity;
        debug_state.num_substeps = config.sim.num_substeps;

        DebugRenderer debug_renderer;
        debug_renderer.Init();
#endif

        // --- main loop ---
        auto  prev_time = std::chrono::high_resolution_clock::now();
        float sim_time = 0.0f;

        while (!window.ShouldClose())
        {
            // delta time
            auto  now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - prev_time).count();
            prev_time = now;
            dt = std::min(dt, 0.033f);

            // matrices
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 proj = camera.GetProjectionMatrix(window.GetAspect());

            // update mouse world position
            input.UpdateMouseWorld(view, proj);

#ifdef TOOP_DEBUG
            debug_ui.BeginFrame();
            debug_state.fps = 1.0f / dt;
            debug_state.frame_ms = dt * 1000.0f;
            debug_state.sphere_speed = sqrtf(
                sphere.GetDeltaX() * sphere.GetDeltaX() +
                sphere.GetDeltaY() * sphere.GetDeltaY() +
                sphere.GetDeltaZ() * sphere.GetDeltaZ()) / dt;
#endif

            window.BeginFrame();

            // --- update ---
            sphere.Update(dt);
#ifdef TOOP_DEBUG
            // mouse ray
            //if (debug_state.show_debug_rays)
            //{
            //    const auto& mouse = window.GetMouseState();
            //    Ray ray = RayUtils::ScreenToRay(
            //        mouse.x, mouse.y,
            //        window.GetWidth(), window.GetHeight(),
            //        view, proj);
            //    debug_renderer.AddRay(ray.origin, ray.direction, 10.0f, glm::vec3(1.0f, 1.0f, 0.0f));
            //}
            if (debug_state.show_debug_rays)
            {
                debug_renderer.AddLine(
                    glm::vec3(-2.0f, 1.5f, 0.0f),
                    glm::vec3(2.0f, 1.5f, 0.0f),
                    glm::vec3(1.0f, 1.0f, 0.0f)); // horizontal yellow line through sphere area
            }

            // drag plane
            if (debug_state.show_drag_plane && input.IsDragging())
            {
                debug_renderer.AddPlane(
                    input.GetDragPlaneCenter(),
                    camera.GetForward(),
                    2.0f, glm::vec3(0.0f, 1.0f, 1.0f)); // cyan
            }

            // sphere velocity arrow
            if (debug_state.show_velocity)
            {
                debug_renderer.AddArrow(
                    glm::vec3(sphere.GetPosX(),
                        sphere.GetPosY(),
                        sphere.GetPosZ()),
                    glm::vec3(sphere.GetDeltaX(),
                        sphere.GetDeltaY(),
                        sphere.GetDeltaZ()),
                    10.0f,
                    glm::vec3(1.0f, 0.0f, 0.0f)); // red
            }

            // light direction
            if (debug_state.show_light)
            {
                debug_renderer.AddArrow(
                    glm::vec3(sphere.GetPosX(),
                        sphere.GetPosY() + 2.0f,
                        sphere.GetPosZ()),
                    glm::vec3(1.0f, 2.0f, 1.0f),
                    1.0f,
                    glm::vec3(1.0f, 1.0f, 0.5f)); // warm yellow
            }
#endif

            sphere.UpdateHeadTilt(
                input.GetMouseWorld().x,
                input.GetMouseWorld().y,
                input.GetMouseWorld().z,
                camera.GetRight().x, camera.GetRight().y, camera.GetRight().z,
                camera.GetUp().x, camera.GetUp().y, camera.GetUp().z);

            sim.SetSphereState(
                sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ(),
                sphere.GetQuatX(), sphere.GetQuatY(), sphere.GetQuatZ(),
                sphere.GetQuatW());

            sim_time += dt;

#ifdef TOOP_DEBUG
            if (!debug_state.freeze_sim)
            {
                sim.SetWind(
                    debug_state.wind_strength * sinf(sim_time * debug_state.wind_frequency),
                    0.0f,
                    debug_state.wind_strength * cosf(sim_time * debug_state.wind_frequency));
                sim.SetNumSubsteps(debug_state.num_substeps);
                sim.SetCompliance(debug_state.compliance);
                sim.SetDamping(debug_state.damping);
                sim.SetGravity(debug_state.gravity);
                sim.AddTime(dt);
                sim.Step(dt, input.IsDragging(),
                    sphere.GetDeltaX(),
                    sphere.GetDeltaY(),
                    sphere.GetDeltaZ());
            }
#else
            sim.SetWind(
                config.sim.wind_strength * sinf(sim_time * config.sim.wind_frequency),
                0.0f,
                config.sim.wind_strength * cosf(sim_time * config.sim.wind_frequency));
            sim.AddTime(dt);
            sim.Step(dt, input.IsDragging(),
                sphere.GetDeltaX(),
                sphere.GetDeltaY(),
                sphere.GetDeltaZ());
#endif

            // --- render ---
            renderer.MapInterop();
            sim.SetInteropBuffer(renderer.GetInteropPtr());
            sim.PackPositionsForRendering();
            renderer.UnmapInterop();

            renderer.Render(
                view, proj,
                glm::vec3(sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ()),
                glm::quat(
                    sphere.GetVisualQuatW(),
                    sphere.GetVisualQuatX(),
                    sphere.GetVisualQuatY(),
                    sphere.GetVisualQuatZ()),
                config.sim.sphere_radius,
                config.bald_patches,
                input.GetMouseWorld());

#ifdef TOOP_DEBUG
            debug_renderer.Render(view, proj);
            debug_ui.Render(debug_state);
            debug_ui.EndFrame();
#endif

            window.EndFrame();
        }

#ifdef TOOP_DEBUG
        debug_renderer.Shutdown();
        debug_ui.Shutdown();
#endif

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
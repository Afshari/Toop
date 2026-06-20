#pragma once

#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include "RayUtils.h"
#include "RaySnapshot.h"

namespace Toop {

    struct DebugContext
    {
        // visibility flags - single source of truth
        bool show_camera_ray = false;
        bool show_eye_planes = false;
        bool show_drag_plane = false;
        bool show_orientation_cubes = false;
        bool show_local_axes = false;
        bool frozen = false;

        // live per-frame data - copied in each frame by DebugManager::Sync
        Ray camera_ray;

        glm::vec3 eye_left_pos = {};
        glm::vec3 eye_left_normal = {};
        glm::vec3 eye_left_hit = {};

        glm::vec3 eye_right_pos = {};
        glm::vec3 eye_right_normal = {};
        glm::vec3 eye_right_hit = {};

        glm::vec3 drag_plane_center = {};
        glm::vec3 drag_plane_normal = {};
        bool      is_dragging = false; // mirrored, not authoritative

        glm::quat rolling_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::quat sphere_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::quat target_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        bool      has_target_orientation = false;

        glm::vec3 sphere_pos = {};
        float sphere_radius = 0.0f;

        std::vector<RaySnapshot> snapshots;
    };

} // namespace Toop

#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS
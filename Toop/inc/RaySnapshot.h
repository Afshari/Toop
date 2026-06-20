#pragma once

#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG

#include <glm/glm.hpp>
#include "RayUtils.h"

namespace Toop {

    // a frozen copy of one frame's ray/eye-plane data, taken by value
    // so it stays valid after the camera/sphere keep moving
    struct RaySnapshot
    {
        Ray camera_ray;

        glm::vec3 eye_left_pos = {};
        glm::vec3 eye_left_normal = {};
        glm::vec3 eye_left_hit = {};

        glm::vec3 eye_right_pos = {};
        glm::vec3 eye_right_normal = {};
        glm::vec3 eye_right_hit = {};
    };

} // namespace Toop

#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS
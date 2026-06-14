#pragma once
#include <glm/glm.hpp>

namespace Toop {

    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    class RayUtils
    {
    public:
        // build ray from mouse screen position
        static Ray ScreenToRay(
            float mouse_x, float mouse_y,
            int   screen_width, int screen_height,
            const glm::mat4& view,
            const glm::mat4& proj);

        // returns true if ray hits sphere
        static bool RaySphereIntersect(
            const Ray& ray,
            const glm::vec3& center,
            float             radius);

        // returns hit point on camera-facing plane through center
        // returns false if no valid intersection
        static bool RayCameraPlaneIntersect(
            const Ray& ray,
            const glm::vec3& center,
            const glm::vec3& camera_forward,
            glm::vec3& hit_out);
    };

} // namespace Toop
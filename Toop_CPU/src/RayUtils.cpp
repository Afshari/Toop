#include "RayUtils.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Toop {

    // --------------------------------------------------------------------------------
    Ray RayUtils::ScreenToRay(
        float            mouse_x,
        float            mouse_y,
        int              screen_width,
        int              screen_height,
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        // normalize to NDC [-1, 1]
        float ndcX = (2.0f * mouse_x / screen_width) - 1.0f;
        float ndcY = -(2.0f * mouse_y / screen_height) + 1.0f;

        glm::mat4 inv_vp = glm::inverse(proj * view);

        glm::vec4 near_pt = inv_vp * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 far_pt = inv_vp * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

        near_pt /= near_pt.w;
        far_pt /= far_pt.w;

        Ray ray;
        ray.origin = glm::vec3(near_pt);
        ray.direction = glm::normalize(glm::vec3(far_pt) - glm::vec3(near_pt));
        return ray;
    }

    // --------------------------------------------------------------------------------
    bool RayUtils::RaySphereIntersect(
        const Ray& ray,
        const glm::vec3& center,
        float            radius)
    {
        glm::vec3 oc = ray.origin - center;
        float b = glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - radius * radius;
        float disc = b * b - c;
        return disc >= 0.0f;
    }

    // --------------------------------------------------------------------------------
    bool RayUtils::RayCameraPlaneIntersect(
        const Ray& ray,
        const glm::vec3& center,
        const glm::vec3& camera_forward,
        glm::vec3& hit_out)
    {
        glm::vec3 plane_normal = glm::normalize(camera_forward);
        float     denom = glm::dot(plane_normal, ray.direction);

        if (fabsf(denom) < 0.0001f)
            return false;

        float t = glm::dot(plane_normal, center - ray.origin) / denom;

        if (t < 0.0f || t > 50.0f)
            return false;

        hit_out = ray.origin + ray.direction * t;
        return true;
    }

} // namespace Toop
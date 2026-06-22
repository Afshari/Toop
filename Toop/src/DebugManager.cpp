#include "DebugManager.h"

#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG

#include "InputHandler.h"
#include <iostream>

namespace Toop {

    // --------------------------------------------------------------------------------
    void DebugManager::Init()
    {
        m_draw = std::make_unique<DebugRenderer>();
        m_draw->Init();
        std::cout << "[INFO] DebugManager initialized." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void DebugManager::Shutdown()
    {
        if (m_draw)
            m_draw->Shutdown();
        m_draw.reset();
        std::cout << "[INFO] DebugManager shutdown." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void DebugManager::Sync(
        const Ray& camera_ray,
        const Camera& camera,
        const SpherePhysics& sphere,
        const Renderer& renderer,
        const InputHandler& input,
        const Config& config)
    {
        m_context.camera_ray = camera_ray;

        glm::vec3 sphere_pos(
            sphere.GetPosX(), sphere.GetPosY(), sphere.GetPosZ());

        m_context.sphere_pos = sphere_pos;
        m_context.sphere_radius = config.sim.sphere_radius;

        glm::quat sphere_orientation(
            sphere.GetVisualQuatW(),
            sphere.GetVisualQuatX(),
            sphere.GetVisualQuatY(),
            sphere.GetVisualQuatZ());

        // eyes - position, outward normal, ray-plane hit point
        m_context.eye_left_pos = renderer.GetEyeWorldPos(
            0, sphere_pos, sphere_orientation,
            config.sim.sphere_radius, config.bald_patches);
        m_context.eye_left_normal = renderer.GetEyeOutwardNormal(
            0, sphere_pos, sphere_orientation,
            config.sim.sphere_radius, config.bald_patches);
        RayUtils::RayCameraPlaneIntersect(
            camera_ray, m_context.eye_left_pos,
            m_context.eye_left_normal, m_context.eye_left_hit);

        m_context.eye_right_pos = renderer.GetEyeWorldPos(
            1, sphere_pos, sphere_orientation,
            config.sim.sphere_radius, config.bald_patches);
        m_context.eye_right_normal = renderer.GetEyeOutwardNormal(
            1, sphere_pos, sphere_orientation,
            config.sim.sphere_radius, config.bald_patches);
        RayUtils::RayCameraPlaneIntersect(
            camera_ray, m_context.eye_right_pos,
            m_context.eye_right_normal, m_context.eye_right_hit);

        // drag plane - mirrored read-only from InputHandler
        m_context.is_dragging = input.IsDragging();
        m_context.drag_plane_center = input.GetDragPlaneCenter();
        m_context.drag_plane_normal = camera.GetForward();

        // orientations
        m_context.rolling_orientation = glm::quat(
            sphere.GetQuatW(), sphere.GetQuatX(),
            sphere.GetQuatY(), sphere.GetQuatZ());
        m_context.sphere_orientation = sphere_orientation;

        // target_orientation / has_target_orientation will be wired up
        // once head tilt is re-implemented with the rolling/target/sphere
        // model - left at defaults for now
    }

    // --------------------------------------------------------------------------------
    void DebugManager::TakeSnapshot()
    {
        RaySnapshot snap;
        snap.camera_ray = m_context.camera_ray;
        snap.eye_left_pos = m_context.eye_left_pos;
        snap.eye_left_normal = m_context.eye_left_normal;
        snap.eye_left_hit = m_context.eye_left_hit;
        snap.eye_right_pos = m_context.eye_right_pos;
        snap.eye_right_normal = m_context.eye_right_normal;
        snap.eye_right_hit = m_context.eye_right_hit;

        m_context.snapshots.push_back(snap);

        if (!m_draw) return;

        m_draw->AddPersistentLine(
            snap.camera_ray.origin,
            snap.camera_ray.origin + glm::normalize(snap.camera_ray.direction) * 20.0f,
            glm::vec3(0.7f, 0.7f, 0.2f)); // dim yellow

        float offset = 0.07f;

        m_draw->AddPersistentFilledPlane(
            snap.eye_left_pos + snap.eye_left_normal * offset,
            snap.eye_left_normal,
            0.3f, glm::vec3(0.1f, 0.6f, 0.1f), 0.3f);

        m_draw->AddPersistentFilledPlane(
            snap.eye_right_pos + snap.eye_right_normal * offset,
            snap.eye_right_normal,
            0.3f, glm::vec3(0.1f, 0.3f, 0.6f), 0.3f);

        m_draw->AddPersistentLine(
            snap.eye_left_pos + snap.eye_left_normal * offset,
            snap.eye_left_hit,
            glm::vec3(0.1f, 0.6f, 0.1f));

        m_draw->AddPersistentLine(
            snap.eye_right_pos + snap.eye_right_normal * offset,
            snap.eye_right_hit,
            glm::vec3(0.1f, 0.3f, 0.6f));

        float marker_size = m_context.sphere_radius * 0.05f;

        m_draw->AddPersistentWireCube(
            snap.eye_left_hit, glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(marker_size), glm::vec3(0.1f, 0.6f, 0.1f));

        m_draw->AddPersistentWireCube(
            snap.eye_right_hit, glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec3(marker_size), glm::vec3(0.1f, 0.3f, 0.6f));
    }

    // --------------------------------------------------------------------------------
    void DebugManager::Draw(const glm::mat4& view, const glm::mat4& proj)
    {
        if (!m_draw) return;

        // camera ray
        if (m_context.show_camera_ray)
        {
            m_draw->AddRay(
                m_context.camera_ray.origin,
                m_context.camera_ray.direction,
                20.0f,
                glm::vec3(1.0f, 1.0f, 0.0f)); // yellow
        }

        // eye planes + hit points
        if (m_context.show_eye_planes)
        {
            float offset = 0.07f;

            m_draw->AddFilledPlane(
                m_context.eye_left_pos + m_context.eye_left_normal * offset,
                m_context.eye_left_normal,
                0.3f, glm::vec3(0.2f, 1.0f, 0.2f), 0.35f);

            m_draw->AddFilledPlane(
                m_context.eye_right_pos + m_context.eye_right_normal * offset,
                m_context.eye_right_normal,
                0.3f, glm::vec3(0.2f, 0.6f, 1.0f), 0.35f);
        }


        // drag plane - only meaningful while actively dragging
        if (m_context.show_drag_plane && m_context.is_dragging)
        {
            m_draw->AddPlane(
                m_context.drag_plane_center, m_context.drag_plane_normal,
                2.0f, glm::vec3(1.0f, 0.4f, 1.0f)); // magenta
        }

        // orientation cubes - rolling (green) / sphere (blue) / target (orange, if active)
        if (m_context.show_orientation_cubes)
        {
            glm::vec3 half_extents(m_context.sphere_radius);

            m_draw->AddWireCube(
                m_context.sphere_pos, m_context.rolling_orientation,
                half_extents, glm::vec3(0.2f, 1.0f, 0.2f)); // green

            m_draw->AddWireCube(
                m_context.sphere_pos, m_context.sphere_orientation,
                half_extents * 1.1f, glm::vec3(0.2f, 0.6f, 1.0f)); // blue, slightly bigger so it doesn't z-fight

            if (m_context.has_target_orientation)
            {
                m_draw->AddWireCube(
                    m_context.sphere_pos, m_context.target_orientation,
                    half_extents * 1.2f, glm::vec3(1.0f, 0.6f, 0.1f)); // orange
            }
        }

        // local axes gizmo - sphere's current local X/Y/Z, rotated by sphere_orientation
        if (m_context.show_local_axes)
        {
            float len = m_context.sphere_radius * 1.5f;

            glm::vec3 axis_x = m_context.sphere_orientation * glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 axis_y = m_context.sphere_orientation * glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 axis_z = m_context.sphere_orientation * glm::vec3(0.0f, 0.0f, 1.0f);

            m_draw->AddLine(m_context.sphere_pos, m_context.sphere_pos + axis_x * len,
                glm::vec3(1.0f, 0.0f, 0.0f)); // red - X
            m_draw->AddLine(m_context.sphere_pos, m_context.sphere_pos + axis_y * len,
                glm::vec3(0.0f, 1.0f, 0.0f)); // green - Y
            m_draw->AddLine(m_context.sphere_pos, m_context.sphere_pos + axis_z * len,
                glm::vec3(0.0f, 0.3f, 1.0f)); // blue - Z
        }

        // hand off to the low-level renderer for actual GL drawing
        m_draw->Render(view, proj);
    }

    // --------------------------------------------------------------------------------
    void DebugManager::ClearSnapshots()
    {
        m_context.snapshots.clear();
        if (m_draw)
        {
            m_draw->ClearPersistent();
            m_draw->ClearPersistentFilled();
        }
    }

} // namespace Toop

#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS
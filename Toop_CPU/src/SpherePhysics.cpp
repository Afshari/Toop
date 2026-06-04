#include "SpherePhysics.h"
#include <cmath>
#include <iostream>

namespace Toop {

    // --------------------------------------------------------------------------------
    static float Length(const Vec3& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    // --------------------------------------------------------------------------------
    static Vec3 Normalize(const Vec3& v)
    {
        float len = Length(v);
        if (len < 1e-6f) return { 0.0f, 0.0f, 0.0f };
        return { v.x / len, v.y / len, v.z / len };
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::Init(
        const SphereConfig& config,
        const RoomConfig& room,
        float               sphere_radius)
    {
        m_gravity = config.gravity;
        m_restitution = config.restitution;
        m_damping = config.damping;
        m_drag_smoothing = config.drag_smoothing;
        m_throw_multiplier = config.throw_multiplier;
        m_idle_threshold = config.idle_threshold_sec;
        m_idle_speed_thresh = config.idle_speed_threshold;
        m_sphere_radius = sphere_radius;
        m_ground_y = room.ground_y;
        m_room_min = { room.min.x, room.min.y, room.min.z };
        m_room_max = { room.max.x, room.max.y, room.max.z };

        // start at sphere_center from sim config
        m_pos = { 0.0f, 1.5f, 0.0f };
        m_vel = { 0.0f, 0.0f, 0.0f };
        m_delta = { 0.0f, 0.0f, 0.0f };
        m_orientation = { 0.0f, 0.0f, 0.0f, 1.0f };

        std::cout << "[INFO] SpherePhysics initialized." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::Update(float dt)
    {
        Vec3 prev_pos = m_pos;

        if (m_is_dragging)
        {
            // smooth toward drag target
            m_pos.x += (m_drag_target.x - m_pos.x) * m_drag_smoothing;
            m_pos.y += (m_drag_target.y - m_pos.y) * m_drag_smoothing;
            m_pos.z += (m_drag_target.z - m_pos.z) * m_drag_smoothing;

            // track drag velocity for throw
            m_drag_velocity.x = (m_pos.x - prev_pos.x) / dt;
            m_drag_velocity.y = (m_pos.y - prev_pos.y) / dt;
            m_drag_velocity.z = (m_pos.z - prev_pos.z) / dt;
        }
        else
        {
            // gravity
            m_vel.y += m_gravity * dt;

            // integrate
            m_pos.x += m_vel.x * dt;
            m_pos.y += m_vel.y * dt;
            m_pos.z += m_vel.z * dt;

            // ground collision
            float floor_y = m_ground_y + m_sphere_radius;
            if (m_pos.y < floor_y)
            {
                m_pos.y = floor_y;
                m_vel.y = -m_vel.y * m_restitution;
                m_vel.x *= m_damping;
                m_vel.z *= m_damping;
            }

            // wall collisions
            float min_x = m_room_min.x + m_sphere_radius;
            float max_x = m_room_max.x - m_sphere_radius;
            float min_z = m_room_min.z + m_sphere_radius;
            float max_z = m_room_max.z - m_sphere_radius;

            if (m_pos.x < min_x) { m_pos.x = min_x; m_vel.x = -m_vel.x * m_restitution; }
            if (m_pos.x > max_x) { m_pos.x = max_x; m_vel.x = -m_vel.x * m_restitution; }
            if (m_pos.z < min_z) { m_pos.z = min_z; m_vel.z = -m_vel.z * m_restitution; }
            if (m_pos.z > max_z) { m_pos.z = max_z; m_vel.z = -m_vel.z * m_restitution; }

            // velocity damping
            m_vel.x *= m_damping;
            m_vel.y *= m_damping;
            m_vel.z *= m_damping;
        }

        // compute delta for hair perturbation
        m_delta.x = m_pos.x - prev_pos.x;
        m_delta.y = m_pos.y - prev_pos.y;
        m_delta.z = m_pos.z - prev_pos.z;

        UpdateOrientation(dt);
        UpdateIdleDetection(dt);
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::StartDrag(float wx, float wy, float wz)
    {
        m_is_dragging = true;
        m_drag_target = { wx, wy, wz };
        m_drag_velocity = { 0.0f, 0.0f, 0.0f };
        m_vel = { 0.0f, 0.0f, 0.0f };
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::UpdateDrag(float wx, float wy, float wz)
    {
        m_drag_target = { wx, wy, wz };
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::EndDrag()
    {
        m_is_dragging = false;
        m_vel.x = m_drag_velocity.x * m_throw_multiplier;
        m_vel.y = m_drag_velocity.y * m_throw_multiplier;
        m_vel.z = m_drag_velocity.z * m_throw_multiplier;
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::UpdateOrientation(float dt)
    {
        float speed = Length(m_vel);
        if (speed < 1e-6f) return;

        // rolling axis = up cross velocity
        Vec3 vel_norm = Normalize(m_vel);
        Vec3 axis = {
            -vel_norm.z,
             0.0f,
             vel_norm.x
        };

        float axis_len = Length(axis);
        if (axis_len < 1e-6f) return;
        axis.x /= axis_len;
        axis.z /= axis_len;

        // angle = arc_length / radius
        float angle = speed * dt / m_sphere_radius;

        // build delta quaternion from axis-angle
        float half = angle * 0.5f;
        float s = sinf(half);
        Quat delta_q = {
            axis.x * s,
            0.0f,
            axis.z * s,
            cosf(half)
        };

        m_orientation = NormalizeQuat(MultiplyQuat(delta_q, m_orientation));
    }

    // --------------------------------------------------------------------------------
    void SpherePhysics::UpdateIdleDetection(float dt)
    {
        float speed = Length(m_vel);
        if (speed < m_idle_speed_thresh)
        {
            m_idle_timer += dt;
            if (m_idle_timer >= m_idle_threshold)
                m_is_idle = true;
        }
        else
        {
            m_idle_timer = 0.0f;
            m_is_idle = false;
        }
    }

    // --------------------------------------------------------------------------------
    Quat SpherePhysics::MultiplyQuat(const Quat& a, const Quat& b) const
    {
        return {
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        };
    }

    // --------------------------------------------------------------------------------
    Quat SpherePhysics::NormalizeQuat(const Quat& q) const
    {
        float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len < 1e-6f) return { 0.0f, 0.0f, 0.0f, 1.0f };
        return { q.x / len, q.y / len, q.z / len, q.w / len };
    }

} // namespace Toop
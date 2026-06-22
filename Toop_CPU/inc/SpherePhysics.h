#pragma once
#include "Config.h"

namespace Toop {

    struct Quat
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;
    };

    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    class SpherePhysics
    {
    public:
        void Init(const SphereConfig& config, const RoomConfig& room, float sphere_radius);
        void Update(float dt);

        // drag interface
        void StartDrag(float world_x, float world_y, float world_z);
        void UpdateDrag(float world_x, float world_y, float world_z);
        void EndDrag();

        // getters for HairSimulator
        float GetPosX()  const { return m_pos.x; }
        float GetPosY()  const { return m_pos.y; }
        float GetPosZ()  const { return m_pos.z; }
        float GetQuatX() const { return m_orientation.x; }
        float GetQuatY() const { return m_orientation.y; }
        float GetQuatZ() const { return m_orientation.z; }
        float GetQuatW() const { return m_orientation.w; }
        bool  IsIdle()   const { return m_is_idle; }

        // delta since last frame - needed by TranslateWithPerturbation
        float GetDeltaX() const { return m_delta.x; }
        float GetDeltaY() const { return m_delta.y; }
        float GetDeltaZ() const { return m_delta.z; }

        void UpdateHeadTilt(
            float mouse_world_x, float mouse_world_y, float mouse_world_z,
            float camera_right_x, float camera_right_y, float camera_right_z,
            float camera_up_x, float camera_up_y, float camera_up_z);

        void RotateTowardCamera(float camera_x, float camera_y, float camera_z);

        float GetVisualQuatX() const { return m_sphere_orientation.x; }
        float GetVisualQuatY() const { return m_sphere_orientation.y; }
        float GetVisualQuatZ() const { return m_sphere_orientation.z; }
        float GetVisualQuatW() const { return m_sphere_orientation.w; }

    private:
        void UpdateOrientation(float dt);
        void UpdateIdleDetection(float dt);
        Quat MultiplyQuat(const Quat& a, const Quat& b) const;
        Quat NormalizeQuat(const Quat& q) const;
        Quat SlerpQuat(const Quat& a, const Quat& b, float t) const;

        // config
        float m_gravity = -1.0f;
        float m_restitution = 0.95f;
        float m_damping = 0.99f;
        float m_drag_smoothing = 0.5f;
        float m_throw_multiplier = 0.3f;
        float m_idle_threshold = 0.5f;
        float m_idle_speed_thresh = 0.05f;
        float m_sphere_radius = 0.4f;
        float m_ground_y = 0.0f;
        Vec3  m_room_min;
        Vec3  m_room_max;
        float m_max_tilt = 0.15f;
        float m_tilt_strength = 0.5f;
        float m_idle_rotate_speed = 0.04f;

        // state
        Vec3  m_pos;
        Vec3  m_vel;
        Vec3  m_delta;
        Quat  m_orientation;

        // drag state
        bool  m_is_dragging = false;
        Vec3  m_drag_target;
        Vec3  m_drag_velocity;

        // idle detection
        bool  m_is_idle = false;
        float m_idle_timer = 0.0f;

        Quat m_sphere_orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

} // namespace Toop
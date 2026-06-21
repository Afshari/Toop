import * as THREE from 'three'

export const PARAMS = {
    // Sphere
    sphere_radius: 0.4,
    sphere_center: [0.0, 1.5, 0.0],
    ground_y: 0.0,

    // Hair geometry
    target_strand_count: 15000,
    num_segments: 5,
    segment_length: 0.035,
    masses: [0.5, 0.5, 0.4, 0.3, 0.2],  // one per non-root particle (index 1..5)

    // PBD simulation
    num_substeps: 15,
    damping: 0.995,
    gravity: -1.0,
    distance_compliance: 0,

    // Eye bald patches - directions in local sphere space (normalized)
    eye_left_dir: new THREE.Vector3(-0.25, 0.15, 1.0).normalize(),
    eye_right_dir: new THREE.Vector3(0.25, 0.15, 1.0).normalize(),
    eye_dot_threshold: 0.97,

    // Sphere physics
    sphere_gravity: -1.0,
    restitution: 0.95,
    sphere_damping: 0.99,
    move_force: 3.0,
    drag_smoothing: 0.5,
    throw_multiplier: 0.3,
    sphere_collision_compliance: 1e-3,

    room_min: [-4.0, 0.0, -4.0],
    room_max: [4.0, 3.0, 4.0],

    max_tilt: 0.30,
    tilt_strength: 0.05,

    idle_speed_threshold: 0.05,
    idle_threshold: 0.5,
    idle_rotate_speed: 0.04,

    wind_strength: 0.03,
    wind_frequency: 0.5,

    eye_white_color: 0xffffff,
    eye_dark_color: 0x111111,
    eye_radius_factor: 0.18,
    eye_offset_factor: 0.85,
    pupil_radius_factor: 0.6,
    pupil_track_strength: 0.4,

    hair_color: new THREE.Color(1.0, 0.5, 0.5),
    sphere_color: new THREE.Color(0.9, 0.15, 0.15),

    whip_strength: 0.002,
}
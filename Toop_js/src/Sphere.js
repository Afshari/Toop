import * as THREE from 'three'
import { PARAMS } from './config.js'

export class Sphere {

    constructor(scene) {
        this.center   = new THREE.Vector3(...PARAMS.sphere_center)
        this.velocity = new THREE.Vector3(0, 0, 0)
        this.radius   = PARAMS.sphere_radius

        this.rollingOrientation = new THREE.Quaternion()  // identity
        this.sphereOrientation  = new THREE.Quaternion()  // identity

        this.roomMin = new THREE.Vector3(...PARAMS.room_min)
        this.roomMax = new THREE.Vector3(...PARAMS.room_max)

        // mesh
        const geo = new THREE.SphereGeometry(this.radius, 32, 32)
        const mat = new THREE.MeshStandardMaterial({ color: 0x555555 })
        this.mesh = new THREE.Mesh(geo, mat)
        this.mesh.castShadow = true
        scene.add(this.mesh)
    }

    update(dt) {
        // damping
        this.velocity.multiplyScalar(PARAMS.sphere_damping)

        // gravity
        this.velocity.y += PARAMS.sphere_gravity * dt

        // integrate position
        this.center.addScaledVector(this.velocity, dt)

        // ground
        if (this.center.y - this.radius < this.roomMin.y) {
            this.center.y = this.roomMin.y + this.radius
            this.velocity.y = -this.velocity.y * PARAMS.restitution
        }
        // ceiling
        if (this.center.y + this.radius > this.roomMax.y) {
            this.center.y = this.roomMax.y - this.radius
            this.velocity.y = -this.velocity.y * PARAMS.restitution
        }
        // left / right
        if (this.center.x - this.radius < this.roomMin.x) {
            this.center.x = this.roomMin.x + this.radius
            this.velocity.x = -this.velocity.x * PARAMS.restitution
        }
        if (this.center.x + this.radius > this.roomMax.x) {
            this.center.x = this.roomMax.x - this.radius
            this.velocity.x = -this.velocity.x * PARAMS.restitution
        }
        // front / back
        if (this.center.z - this.radius < this.roomMin.z) {
            this.center.z = this.roomMin.z + this.radius
            this.velocity.z = -this.velocity.z * PARAMS.restitution
        }
        if (this.center.z + this.radius > this.roomMax.z) {
            this.center.z = this.roomMax.z - this.radius
            this.velocity.z = -this.velocity.z * PARAMS.restitution
        }

        this.mesh.position.copy(this.center)
    }

    updateOrientation(dt) {
        const vx = this.velocity.x
        const vy = this.velocity.y
        const vz = this.velocity.z

        const ax    = vz / this.radius
        const ay    = -vx / this.radius
        const az    = -vy / this.radius
        const angle = Math.sqrt(ax * ax + ay * ay + az * az) * dt

        if (angle > 1e-6) {
            const axis  = new THREE.Vector3(ax, ay, az).normalize()
            const delta = new THREE.Quaternion().setFromAxisAngle(axis, angle)
            this.rollingOrientation.premultiply(delta).normalize()
        }

        this.sphereOrientation.copy(this.rollingOrientation)
    }

    getCenter()      { return this.center }
    getOrientation() { return this.sphereOrientation }

    dispose() {
        this.mesh.geometry.dispose()
        this.mesh.material.dispose()
    }
}
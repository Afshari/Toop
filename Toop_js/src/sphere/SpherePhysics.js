import * as THREE from 'three'
import { PARAMS } from '../config.js'

export class SpherePhysics {

    constructor(center, radius, roomMin, roomMax) {
        this.center = center
        this.radius = radius
        this.roomMin = roomMin
        this.roomMax = roomMax

        this.velocity = new THREE.Vector3(0, 0, 0)
        this.isDragging = false
        this.dragVelocity = new THREE.Vector3()
        this._frameDelta = new THREE.Vector3()
        this._prevCenter = center.clone()
        this._dragHit = new THREE.Vector3()
        this._lastHit = new THREE.Vector3()
    }

    update(dt) {
        if (this.isDragging) {
            this.dragVelocity.copy(this._lastHit).sub(this.center).divideScalar(dt)
        }

        if (!this.isDragging) {
            this.velocity.multiplyScalar(PARAMS.sphere_damping)
            this.velocity.y += PARAMS.sphere_gravity * dt
            this.center.addScaledVector(this.velocity, dt)
            this._resolveWallCollisions()
        }

        this._frameDelta.copy(this.center).sub(this._prevCenter)
        this._prevCenter.copy(this.center)
    }

    _resolveWallCollisions() {
        const { restitution } = PARAMS
        const r = this.radius

        if (this.center.y - r < this.roomMin.y) {
            this.center.y = this.roomMin.y + r
            this.velocity.y = -this.velocity.y * restitution
        }
        if (this.center.y + r > this.roomMax.y) {
            this.center.y = this.roomMax.y - r
            this.velocity.y = -this.velocity.y * restitution
        }
        if (this.center.x - r < this.roomMin.x) {
            this.center.x = this.roomMin.x + r
            this.velocity.x = -this.velocity.x * restitution
        }
        if (this.center.x + r > this.roomMax.x) {
            this.center.x = this.roomMax.x - r
            this.velocity.x = -this.velocity.x * restitution
        }
        if (this.center.z - r < this.roomMin.z) {
            this.center.z = this.roomMin.z + r
            this.velocity.z = -this.velocity.z * restitution
        }
        if (this.center.z + r > this.roomMax.z) {
            this.center.z = this.roomMax.z - r
            this.velocity.z = -this.velocity.z * restitution
        }
    }

    handleDragStart(raycasterContext, ray) {
        const hit = raycasterContext.intersectSphere(ray, this.center, this.radius)
        if (!hit) return false
        this.isDragging = true
        this.velocity.set(0, 0, 0)
        this.dragVelocity.set(0, 0, 0)
        return true
    }

    handleDragMove(raycasterContext, ray, camera) {
        const forward = new THREE.Vector3()
        camera.getWorldDirection(forward)

        const hit = raycasterContext.intersectPlane(ray, forward, this.center)
        if (!hit) return

        this._dragHit.copy(hit.point)
        this.center.lerp(this._dragHit, PARAMS.drag_smoothing)
        this._lastHit.copy(this._dragHit)

        this.center.x = Math.max(this.roomMin.x + this.radius, Math.min(this.roomMax.x - this.radius, this.center.x))
        this.center.y = Math.max(this.roomMin.y + this.radius, Math.min(this.roomMax.y - this.radius, this.center.y))
        this.center.z = Math.max(this.roomMin.z + this.radius, Math.min(this.roomMax.z - this.radius, this.center.z))
    }

    handleDragEnd() {
        this.isDragging = false
        this.velocity.copy(this.dragVelocity).multiplyScalar(PARAMS.throw_multiplier)
    }

    getFrameDelta() { return this._frameDelta }
}
import * as THREE from 'three'
import { PARAMS } from '../config.js'

export class SphereCharacter {

    constructor() {
        this.rollingOrientation = new THREE.Quaternion()
        this.sphereOrientation = new THREE.Quaternion()

        this.idleTimer = 0.0

        // pre-allocated
        this._forward = new THREE.Vector3()
        this._targetQuat = new THREE.Quaternion()
        this._tiltX = new THREE.Quaternion()
        this._tiltY = new THREE.Quaternion()
        this._axisY = new THREE.Vector3(0, 1, 0)
        this._axisX = new THREE.Vector3(1, 0, 0)
    }

    updateOrientation(velocity, radius, dt) {
        const vx = velocity.x
        const vy = velocity.y
        const vz = velocity.z

        const ax = vz / radius
        const ay = -vx / radius
        const az = -vy / radius
        const angle = Math.sqrt(ax * ax + ay * ay + az * az) * dt

        if (angle > 1e-6) {
            const axis = new THREE.Vector3(ax, ay, az).normalize()
            const delta = new THREE.Quaternion().setFromAxisAngle(axis, angle)
            this.rollingOrientation.premultiply(delta).normalize()
        }

        this.sphereOrientation.copy(this.rollingOrientation)
    }

    updateIdleTimer(velocity, dt) {
        if (velocity.length() < PARAMS.idle_speed_threshold) {
            this.idleTimer += dt
        } else {
            this.idleTimer = 0.0
        }
    }

    isIdle() {
        return this.idleTimer >= PARAMS.idle_threshold
    }

    rotateTowardCamera(cameraPos, center) {
        if (!this.isIdle()) return

        const toCamera = this._forward.set(
            cameraPos.x - center.x,
            0.0,
            cameraPos.z - center.z
        ).normalize()

        const angleToCamera = Math.atan2(toCamera.x, toCamera.z)
        this._targetQuat.setFromAxisAngle(this._axisY, angleToCamera)

        this.rollingOrientation.slerp(this._targetQuat, Math.min(PARAMS.idle_rotate_speed, 1.0))
        this.rollingOrientation.normalize()
    }

    updateHeadTilt(mouseNDC) {
        if (!this.isIdle()) {
            this.sphereOrientation.slerp(this.rollingOrientation, 0.15)
            return
        }

        const { max_tilt } = PARAMS

        const tiltX = -mouseNDC.y * max_tilt
        const tiltY = mouseNDC.x * max_tilt

        this._tiltY.setFromAxisAngle(this._axisY, tiltY)
        this._tiltX.setFromAxisAngle(this._axisX, tiltX)

        this._targetQuat
            .copy(this.rollingOrientation)
            .multiply(this._tiltY)
            .multiply(this._tiltX)
            .normalize()

        this.sphereOrientation.slerp(this._targetQuat, 0.5)
    }

    getOrientation() { return this.sphereOrientation }
}
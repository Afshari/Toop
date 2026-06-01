import * as THREE from 'three'
import { PARAMS } from './config.js'

export class Sphere {

    constructor(scene) {
        this.center = new THREE.Vector3(...PARAMS.sphere_center)
        this.velocity = new THREE.Vector3(0, 0, 0)
        this.radius = PARAMS.sphere_radius

        this.rollingOrientation = new THREE.Quaternion()  // identity
        this.sphereOrientation = new THREE.Quaternion()  // identity

        this.roomMin = new THREE.Vector3(...PARAMS.room_min)
        this.roomMax = new THREE.Vector3(...PARAMS.room_max)

        this.idleTimer = 0.0

        // pre-allocated objects for character methods
        this._raycaster = new THREE.Raycaster()
        this._plane = new THREE.Plane()
        this._hitPoint = new THREE.Vector3()
        this._cameraRight = new THREE.Vector3()
        this._cameraUp = new THREE.Vector3()
        this._offset = new THREE.Vector3()
        this._forward = new THREE.Vector3()
        this._targetQuat = new THREE.Quaternion()
        this._tiltX = new THREE.Quaternion()
        this._tiltY = new THREE.Quaternion()

        this._axisY = new THREE.Vector3(0, 1, 0)
        this._axisX = new THREE.Vector3(1, 0, 0)

        this.isDragging = false
        this.dragVelocity = new THREE.Vector3()
        this._frameDelta = new THREE.Vector3()
        this._prevCenter = new THREE.Vector3(...PARAMS.sphere_center)
        this._dragHit = new THREE.Vector3()

        // eye meshes
        const eyeR = this.radius * PARAMS.eye_radius_factor
        const eyeGeo = new THREE.SphereGeometry(eyeR, 16, 16)
        const eyeMat = new THREE.MeshStandardMaterial({
            color: PARAMS.eye_white_color,
            emissive: 0xffffff,
            emissiveIntensity: 0.3,
        })
        this.leftEyeMesh = new THREE.Mesh(eyeGeo, eyeMat)
        this.rightEyeMesh = new THREE.Mesh(eyeGeo, eyeMat.clone())
        scene.add(this.leftEyeMesh)
        scene.add(this.rightEyeMesh)

        const pupilR = eyeR * PARAMS.pupil_radius_factor
        const pupilGeo = new THREE.SphereGeometry(pupilR, 32, 32)
        const pupilMat = new THREE.MeshStandardMaterial({ color: PARAMS.eye_dark_color })
        this.leftPupilMesh = new THREE.Mesh(pupilGeo, pupilMat)
        this.rightPupilMesh = new THREE.Mesh(pupilGeo, pupilMat.clone())
        scene.add(this.leftPupilMesh)
        scene.add(this.rightPupilMesh)

        // eye local directions (unit vectors, rotated by sphereOrientation each frame)
        this._leftEyeDir = new THREE.Vector3(-0.25, 0.15, 1.0).normalize()
        this._rightEyeDir = new THREE.Vector3(0.25, 0.15, 1.0).normalize()

        // pre-allocated for updateEyes
        this._eyeLocalVec = new THREE.Vector3()
        this._eyeOutward = new THREE.Vector3()
        this._lookDir = new THREE.Vector3()
        this._finalDir = new THREE.Vector3()
        this._pupilPos = new THREE.Vector3()
        this._leftEyeWorld = new THREE.Vector3()
        this._rightEyeWorld = new THREE.Vector3()

        // mesh
        const geo = new THREE.SphereGeometry(this.radius, 32, 32)
        const mat = new THREE.MeshStandardMaterial({ color: PARAMS.sphere_color })
        this.mesh = new THREE.Mesh(geo, mat)
        this.mesh.castShadow = true
        scene.add(this.mesh)
    }

    update(dt) {
        this._frameDelta.set(0, 0, 0)

        if (!this.isDragging) {
            this.velocity.multiplyScalar(PARAMS.sphere_damping)
            this.velocity.y += PARAMS.sphere_gravity * dt
            this.center.addScaledVector(this.velocity, dt)

            // all wall/floor collisions stay here unchanged
            if (this.center.y - this.radius < this.roomMin.y) {
                this.center.y = this.roomMin.y + this.radius
                this.velocity.y = -this.velocity.y * PARAMS.restitution
            }
            if (this.center.y + this.radius > this.roomMax.y) {
                this.center.y = this.roomMax.y - this.radius
                this.velocity.y = -this.velocity.y * PARAMS.restitution
            }
            if (this.center.x - this.radius < this.roomMin.x) {
                this.center.x = this.roomMin.x + this.radius
                this.velocity.x = -this.velocity.x * PARAMS.restitution
            }
            if (this.center.x + this.radius > this.roomMax.x) {
                this.center.x = this.roomMax.x - this.radius
                this.velocity.x = -this.velocity.x * PARAMS.restitution
            }
            if (this.center.z - this.radius < this.roomMin.z) {
                this.center.z = this.roomMin.z + this.radius
                this.velocity.z = -this.velocity.z * PARAMS.restitution
            }
            if (this.center.z + this.radius > this.roomMax.z) {
                this.center.z = this.roomMax.z - this.radius
                this.velocity.z = -this.velocity.z * PARAMS.restitution
            }
        }

        this._frameDelta.copy(this.center).sub(this._prevCenter)
        this._prevCenter.copy(this.center)
        this.mesh.position.copy(this.center)
    }

    handleDragStart(raycaster) {
        const hits = raycaster.intersectObject(this.mesh)
        if (hits.length === 0) return false
        this.isDragging = true
        this.velocity.set(0, 0, 0)
        this.dragVelocity.set(0, 0, 0)
        return true
    }

    handleDragMove(raycaster, camera, dt) {
        camera.getWorldDirection(this._forward)
        this._plane.setFromNormalAndCoplanarPoint(this._forward, this.center)
        const hit = raycaster.ray.intersectPlane(this._plane, this._dragHit)
        if (!hit) return

        this.dragVelocity.copy(hit).sub(this.center).divideScalar(dt)
        this.center.lerp(hit, PARAMS.drag_smoothing)

        this.center.x = Math.max(this.roomMin.x + this.radius, Math.min(this.roomMax.x - this.radius, this.center.x))
        this.center.y = Math.max(this.roomMin.y + this.radius, Math.min(this.roomMax.y - this.radius, this.center.y))
        this.center.z = Math.max(this.roomMin.z + this.radius, Math.min(this.roomMax.z - this.radius, this.center.z))
    }

    handleDragEnd() {
        this.isDragging = false
        this.velocity.copy(this.dragVelocity).multiplyScalar(PARAMS.throw_multiplier)
    }

    updateEyes(camera, mouseNDC) {
        const eyeR = this.radius * PARAMS.eye_radius_factor
        const eyeOffset = this.radius * PARAMS.eye_offset_factor

        const eyes = [
            {
                dir: this._leftEyeDir, eyeMesh: this.leftEyeMesh,
                pupilMesh: this.leftPupilMesh, worldPos: this._leftEyeWorld
            },
            {
                dir: this._rightEyeDir, eyeMesh: this.rightEyeMesh,
                pupilMesh: this.rightPupilMesh, worldPos: this._rightEyeWorld
            },
        ]

        for (const eye of eyes) {
            // eye world position
            this._eyeLocalVec.copy(eye.dir).multiplyScalar(eyeOffset)
                .applyQuaternion(this.sphereOrientation)
            eye.worldPos.copy(this.center).add(this._eyeLocalVec)
            eye.eyeMesh.position.copy(eye.worldPos)

            // outward normal of eye in world space
            this._eyeOutward.copy(eye.dir)
                .applyQuaternion(this.sphereOrientation).normalize()

            // default pupil - centered facing outward
            this._pupilPos.copy(eye.worldPos)
                .addScaledVector(this._eyeOutward, eyeR * 0.55)

            // ray-plane intersection for pupil tracking
            this._raycaster.setFromCamera(mouseNDC, camera)
            this._plane.setFromNormalAndCoplanarPoint(this._eyeOutward, eye.worldPos)
            const hit = this._raycaster.ray.intersectPlane(this._plane, this._hitPoint)

            if (hit) {
                this._lookDir.copy(hit).sub(eye.worldPos)
                const dist = this._lookDir.length()
                if (dist > 1e-6) {
                    this._lookDir.divideScalar(dist)
                    // blend outward + look - keeps pupil on front hemisphere
                    this._finalDir.copy(this._eyeOutward)
                        .addScaledVector(this._lookDir, PARAMS.pupil_track_strength)
                        .normalize()
                    this._pupilPos.copy(eye.worldPos)
                        .addScaledVector(this._finalDir, eyeR * 0.55)
                }
            }

            eye.pupilMesh.position.copy(this._pupilPos)
        }
    }

    getFrameDelta() { return this._frameDelta }

    updateOrientation(dt) {
        const vx = this.velocity.x
        const vy = this.velocity.y
        const vz = this.velocity.z

        const ax = vz / this.radius
        const ay = -vx / this.radius
        const az = -vy / this.radius
        const angle = Math.sqrt(ax * ax + ay * ay + az * az) * dt

        if (angle > 1e-6) {
            const axis = new THREE.Vector3(ax, ay, az).normalize()
            const delta = new THREE.Quaternion().setFromAxisAngle(axis, angle)
            this.rollingOrientation.premultiply(delta).normalize()
        }

        this.sphereOrientation.copy(this.rollingOrientation)
    }

    isIdle() {
        return this.velocity.length() < PARAMS.idle_speed_threshold
    }

    isTrulyIdle() {
        return this.idleTimer >= PARAMS.idle_threshold
    }

    updateIdleTimer(dt) {
        if (this.isIdle()) {
            this.idleTimer += dt
        } else {
            this.idleTimer = 0.0
        }
    }

    rotateTowardCamera(cameraPos) {
        if (!this.isTrulyIdle()) return

        const toCamera = this._forward.set(
            cameraPos.x - this.center.x,
            0.0,
            cameraPos.z - this.center.z
        ).normalize()

        const angleToCamera = Math.atan2(toCamera.x, toCamera.z)
        this._targetQuat.setFromAxisAngle(this._axisY, angleToCamera)

        this.rollingOrientation.slerp(this._targetQuat, Math.min(PARAMS.idle_rotate_speed, 1.0))
        this.rollingOrientation.normalize()
    }

    updateHeadTilt(camera, mouseNDC) {
        if (!this.isTrulyIdle()) {
            this.sphereOrientation.copy(this.rollingOrientation)
            return
        }

        // ray from camera through mouse position
        this._raycaster.setFromCamera(mouseNDC, camera)

        // plane through sphere center perpendicular to camera view direction
        camera.getWorldDirection(this._forward)
        this._plane.setFromNormalAndCoplanarPoint(this._forward, this.center)

        const hit = this._raycaster.ray.intersectPlane(this._plane, this._hitPoint)
        if (!hit) {
            this.sphereOrientation.copy(this.rollingOrientation)
            return
        }

        // offset from sphere center in camera right / up space
        this._cameraRight.setFromMatrixColumn(camera.matrixWorld, 0).normalize()
        this._cameraUp.setFromMatrixColumn(camera.matrixWorld, 1).normalize()

        this._offset.copy(hit).sub(this.center)
        const dx = this._offset.dot(this._cameraRight)
        const dy = this._offset.dot(this._cameraUp)

        const { max_tilt, tilt_strength } = PARAMS
        const tiltX = Math.max(-max_tilt, Math.min(max_tilt, -dy * tilt_strength))
        const tiltY = Math.max(-max_tilt, Math.min(max_tilt, dx * tilt_strength))

        this._tiltY.setFromAxisAngle(this._axisY, tiltY)
        this._tiltX.setFromAxisAngle(this._axisX, tiltX)

        // sphereOrientation = rolling * tilt (hair roots follow tilt too)
        this.sphereOrientation
            .copy(this.rollingOrientation)
            .multiply(this._tiltY)
            .multiply(this._tiltX)
            .normalize()
    }

    getCenter() { return this.center }
    getOrientation() { return this.sphereOrientation }

    dispose() {
        this.mesh.geometry.dispose()
        this.mesh.material.dispose()
    }
}
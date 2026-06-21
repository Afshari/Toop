import * as THREE from 'three'
import { PARAMS } from '../config.js'

export class SphereEyes {

    constructor(scene, radius) {
        this._scene = scene
        this._radius = radius

        // pre-allocated
        this._eyeLocalVec = new THREE.Vector3()
        this._eyeOutward = new THREE.Vector3()
        this._lookDir = new THREE.Vector3()
        this._finalDir = new THREE.Vector3()
        this._pupilPos = new THREE.Vector3()
        this._leftEyeWorld = new THREE.Vector3()
        this._rightEyeWorld = new THREE.Vector3()

        // eye local directions
        this._leftEyeDir = new THREE.Vector3(-0.25, 0.15, 1.0).normalize()
        this._rightEyeDir = new THREE.Vector3(0.25, 0.15, 1.0).normalize()

        this._lastOrientation = new THREE.Quaternion()

        this._eyes = this._createEyes()
    }

    _createEyes() {
        const eyeR = this._radius * PARAMS.eye_radius_factor
        const eyeGeo = new THREE.SphereGeometry(eyeR, 16, 16)
        const eyeMat = new THREE.MeshStandardMaterial({
            color: PARAMS.eye_white_color,
            emissive: 0xffffff,
            emissiveIntensity: 0.3,
        })

        const leftEyeMesh = new THREE.Mesh(eyeGeo, eyeMat)
        const rightEyeMesh = new THREE.Mesh(eyeGeo, eyeMat.clone())
        this._scene.add(leftEyeMesh)
        this._scene.add(rightEyeMesh)

        const pupilR = eyeR * PARAMS.pupil_radius_factor
        const pupilGeo = new THREE.SphereGeometry(pupilR, 32, 32)
        const pupilMat = new THREE.MeshStandardMaterial({ color: PARAMS.eye_dark_color })

        const leftPupilMesh = new THREE.Mesh(pupilGeo, pupilMat)
        const rightPupilMesh = new THREE.Mesh(pupilGeo, pupilMat.clone())
        this._scene.add(leftPupilMesh)
        this._scene.add(rightPupilMesh)

        return [
            {
                dir: this._leftEyeDir,
                eyeMesh: leftEyeMesh,
                pupilMesh: leftPupilMesh,
                worldPos: this._leftEyeWorld
            },
            {
                dir: this._rightEyeDir,
                eyeMesh: rightEyeMesh,
                pupilMesh: rightPupilMesh,
                worldPos: this._rightEyeWorld
            },
        ]
    }

    update(center, orientation, raycasterContext, mouseNDC, camera) {
        const eyeR = this._radius * PARAMS.eye_radius_factor
        const eyeOffset = this._radius * PARAMS.eye_offset_factor
        this._lastOrientation.copy(orientation)

        for (const eye of this._eyes) {
            // eye world position
            this._eyeLocalVec.copy(eye.dir)
                .multiplyScalar(eyeOffset)
                .applyQuaternion(orientation)
            eye.worldPos.copy(center).add(this._eyeLocalVec)
            eye.eyeMesh.position.copy(eye.worldPos)

            // outward normal of eye in world space
            this._eyeOutward.copy(eye.dir)
                .applyQuaternion(orientation)
                .normalize()

            // default pupil - centered facing outward
            this._pupilPos.copy(eye.worldPos)
                .addScaledVector(this._eyeOutward, eyeR * 0.55)

            // ray-plane intersection for pupil tracking
            const ray = raycasterContext.castRay(mouseNDC, camera)
            const hit = raycasterContext.intersectPlane(ray, this._eyeOutward, eye.worldPos)

            if (hit) {
                this._lookDir.copy(hit.point).sub(eye.worldPos)
                const dist = this._lookDir.length()
                if (dist > 1e-6) {
                    this._lookDir.divideScalar(dist)
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

    getEyes() { return this._eyes }

    getEyePlanes() {
        return this._eyes.map(eye => ({
            normal: new THREE.Vector3()
                .copy(eye.dir)
                .applyQuaternion(this._lastOrientation)
                .normalize(),
            point: eye.worldPos.clone()
        }))
    }

    dispose() {
        for (const eye of this._eyes) {
            eye.eyeMesh.geometry.dispose()
            eye.eyeMesh.material.dispose()
            eye.pupilMesh.geometry.dispose()
            eye.pupilMesh.material.dispose()
        }
    }
}
import * as THREE from 'three'
import { PARAMS } from '../config.js'
import { SpherePhysics } from './SpherePhysics.js'
import { SphereCharacter } from './SphereCharacter.js'
import { SphereEyes } from './SphereEyes.js'

export class Sphere {

    constructor(scene, raycasterContext) {
        this._scene = scene
        this._raycasterContext = raycasterContext

        this.radius = PARAMS.sphere_radius
        this.center = new THREE.Vector3(...PARAMS.sphere_center)

        const roomMin = new THREE.Vector3(...PARAMS.room_min)
        const roomMax = new THREE.Vector3(...PARAMS.room_max)

        // modules
        this.physics = new SpherePhysics(this.center, this.radius, roomMin, roomMax)
        this.character = new SphereCharacter()
        this.eyes = new SphereEyes(scene, this.radius)

        // mesh
        const geo = new THREE.SphereGeometry(this.radius, 32, 32)
        const mat = new THREE.MeshStandardMaterial({ color: PARAMS.sphere_color })
        this.mesh = new THREE.Mesh(geo, mat)
        this.mesh.castShadow = true
        scene.add(this.mesh)

        // debug helpers
        this._initDebugHelpers(scene)
    }

    _initDebugHelpers(scene) {
        // local axes
        this.axesHelper = new THREE.AxesHelper(this.radius * 2)
        scene.add(this.axesHelper)

        // live ray line
        this.rayLine = new THREE.Line(
            new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(),
                new THREE.Vector3()
            ]),
            new THREE.LineBasicMaterial({ color: 0xff00ff })
        )
        scene.add(this.rayLine)

        // ray hit marker
        this.rayHitMarker = new THREE.Mesh(
            new THREE.SphereGeometry(0.03, 8, 8),
            new THREE.MeshBasicMaterial({ color: 0x00ffff })
        )
        scene.add(this.rayHitMarker)

        // drag plane
        this.dragPlaneMesh = new THREE.Mesh(
            new THREE.PlaneGeometry(2, 2),
            new THREE.MeshBasicMaterial({
                color: 0x4444ff,
                transparent: true,
                opacity: 0.3,
                side: THREE.DoubleSide,
            })
        )
        this.dragPlaneMesh.visible = false
        scene.add(this.dragPlaneMesh)

        // saved ray lines
        this.savedRayLines = []
        this._savedRayMaterial = new THREE.LineBasicMaterial({ color: 0xffff00 })

        // pre-allocated for debug plane orientation
        this._planeAlignQuat = new THREE.Quaternion()
        this._planeDefaultNormal = new THREE.Vector3(0, 0, 1)
        this._cameraForward = new THREE.Vector3()
    }

    // ------------------------------------------------------------
    // per-frame update
    // ------------------------------------------------------------
    update(dt) {
        this.physics.update(dt)
        this.mesh.position.copy(this.center)
        this.axesHelper.position.copy(this.center)
        this.axesHelper.quaternion.copy(this.character.getOrientation())
    }

    updateIdleTimer(dt) {
        this.character.updateIdleTimer(this.physics.velocity, dt)
    }

    updateOrientation(dt) {
        this.character.updateOrientation(this.physics.velocity, this.radius, dt)
    }

    rotateTowardCamera(cameraPos) {
        this.character.rotateTowardCamera(cameraPos, this.center)
    }

    updateHeadTilt(mouseNDC) {
        this.character.updateHeadTilt(mouseNDC)
    }

    updateEyes(mouseNDC, camera) {
        this.eyes.update(
            this.center,
            this.character.getOrientation(),
            this._raycasterContext,
            mouseNDC,
            camera
        )
    }

    // ------------------------------------------------------------
    // drag
    // ------------------------------------------------------------
    handleDragStart(ray) {
        return this.physics.handleDragStart(this._raycasterContext, ray)
    }

    handleDragMove(ray, camera) {
        this.physics.handleDragMove(this._raycasterContext, ray, camera)

        // update debug plane
        camera.getWorldDirection(this._cameraForward)
        this.dragPlaneMesh.position.copy(this.center)
        this._planeAlignQuat.setFromUnitVectors(this._planeDefaultNormal, this._cameraForward)
        this.dragPlaneMesh.quaternion.copy(this._planeAlignQuat)
        this.dragPlaneMesh.visible = true
    }

    handleDragEnd() {
        this.physics.handleDragEnd()
        this.dragPlaneMesh.visible = false
    }

    // ------------------------------------------------------------
    // debug
    // ------------------------------------------------------------
    debugUpdateRay(ray) {
        const origin = ray.origin
        const dir = ray.dir
        const far = origin.clone().addScaledVector(dir, 5)

        const positions = this.rayLine.geometry.attributes.position
        positions.setXYZ(0, origin.x, origin.y, origin.z)
        positions.setXYZ(1, far.x, far.y, far.z)
        positions.needsUpdate = true

        const hit = this._raycasterContext.intersectSphere(ray, this.center, this.radius)
        if (hit) {
            this.rayHitMarker.position.copy(hit.point)
            this.rayHitMarker.visible = true
        } else {
            this.rayHitMarker.visible = false
        }
    }

    saveCurrentRay(ray) {
        const origin = ray.origin.clone()
        const far = origin.clone().addScaledVector(ray.dir, 20)
        const geometry = new THREE.BufferGeometry().setFromPoints([origin, far])
        const line = new THREE.Line(geometry, this._savedRayMaterial)
        this._scene.add(line)
        this.savedRayLines.push(line)
    }

    clearSavedRays() {
        for (const line of this.savedRayLines) {
            this._scene.remove(line)
            line.geometry.dispose()
        }
        this.savedRayLines.length = 0
    }

    // ------------------------------------------------------------
    // getters
    // ------------------------------------------------------------
    get isDragging() { return this.physics.isDragging }
    getCenter() { return this.center }
    getOrientation() { return this.character.getOrientation() }
    getFrameDelta() { return this.physics.getFrameDelta() }

    dispose() {
        this.mesh.geometry.dispose()
        this.mesh.material.dispose()
        this.eyes.dispose()
    }
}
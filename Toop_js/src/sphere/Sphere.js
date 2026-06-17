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

        this.frozen = false

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
    }

    // ------------------------------------------------------------
    // per-frame update
    // ------------------------------------------------------------
    update(dt) {
        this.physics.update(dt)
        this.mesh.position.copy(this.center)
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
    }

    handleDragEnd() {
        this.physics.handleDragEnd()
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
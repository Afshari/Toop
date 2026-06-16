import * as THREE from 'three'
import { RaycasterStrategy } from './RaycasterStrategy.js'

export class ThreeRaycaster extends RaycasterStrategy {

    constructor() {
        super()
        this._raycaster = new THREE.Raycaster()
        this._sphereMesh = null
        this._origin = new THREE.Vector3()
        this._dir = new THREE.Vector3()
        this._hitPoint = new THREE.Vector3()
        this._plane = new THREE.Plane()
    }

    // sphere mesh reference needed for intersectObject call
    setSphereMesh(mesh) {
        this._sphereMesh = mesh
    }

    castRay(mouseNDC, camera) {
        this._raycaster.setFromCamera(mouseNDC, camera)
        this._origin.copy(this._raycaster.ray.origin)
        this._dir.copy(this._raycaster.ray.direction)
        return { origin: this._origin, dir: this._dir }
    }

    intersectSphere(ray, center, radius) {
        if (!this._sphereMesh) {
            throw new Error('ThreeRaycaster: call setSphereMesh() before intersectSphere()')
        }

        const hits = this._raycaster.intersectObject(this._sphereMesh)
        if (hits.length === 0) return null

        return {
            t: hits[0].distance,
            point: hits[0].point.clone()
        }
    }

    intersectPlane(ray, normal, point) {
        this._plane.setFromNormalAndCoplanarPoint(normal, point)
        const hit = this._raycaster.ray.intersectPlane(this._plane, this._hitPoint)
        if (!hit) return null

        return {
            t: ray.origin.distanceTo(this._hitPoint),
            point: this._hitPoint.clone()
        }
    }
}
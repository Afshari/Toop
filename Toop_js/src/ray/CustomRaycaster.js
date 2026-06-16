import * as THREE from 'three'
import { RaycasterStrategy } from './RaycasterStrategy.js'

export class CustomRaycaster extends RaycasterStrategy {

    constructor() {
        super()
        // pre-allocated internals
        this._origin = new THREE.Vector3()
        this._dir = new THREE.Vector3()
    }

    castRay(mouseNDC, camera) {
        this._origin.copy(camera.position)

        this._dir.set(mouseNDC.x, mouseNDC.y, 0.5)
        this._dir.unproject(camera)
        this._dir.sub(this._origin)
        this._dir.normalize()

        return { origin: this._origin, dir: this._dir }
    }

    intersectSphere(ray, center, radius) {
        const ocx = ray.origin.x - center.x
        const ocy = ray.origin.y - center.y
        const ocz = ray.origin.z - center.z

        const b = ocx * ray.dir.x + ocy * ray.dir.y + ocz * ray.dir.z
        const c = (ocx * ocx + ocy * ocy + ocz * ocz) - radius * radius
        const disc = b * b - c

        if (disc < 0) return null

        const sqrtDisc = Math.sqrt(disc)
        const t1 = -b - sqrtDisc
        const t2 = -b + sqrtDisc

        let t = t1
        if (t < 0) t = t2
        if (t < 0) return null

        return {
            t,
            point: new THREE.Vector3(
                ray.origin.x + ray.dir.x * t,
                ray.origin.y + ray.dir.y * t,
                ray.origin.z + ray.dir.z * t,
            )
        }
    }

    intersectPlane(ray, normal, point) {
        const denom = ray.dir.x * normal.x + ray.dir.y * normal.y + ray.dir.z * normal.z

        if (Math.abs(denom) < 1e-6) return null

        const diffx = point.x - ray.origin.x
        const diffy = point.y - ray.origin.y
        const diffz = point.z - ray.origin.z

        const t = (diffx * normal.x + diffy * normal.y + diffz * normal.z) / denom

        if (t < 0) return null

        return {
            t,
            point: new THREE.Vector3(
                ray.origin.x + ray.dir.x * t,
                ray.origin.y + ray.dir.y * t,
                ray.origin.z + ray.dir.z * t,
            )
        }
    }
}
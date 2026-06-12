
import * as THREE from 'three'

const _dirScratch = new THREE.Vector3()

export function rayFromCamera(mouseNDC, camera, outOrigin) {
    const origin = outOrigin || new THREE.Vector3()
    origin.copy(camera.position)

    _dirScratch.set(mouseNDC.x, mouseNDC.y, 0.5)
    _dirScratch.unproject(camera)
    _dirScratch.sub(origin)
    _dirScratch.normalize()

    return { origin, dir: _dirScratch }
}

export function raySphereIntersect(origin, dir, center, radius) {
    const ocx = origin.x - center.x
    const ocy = origin.y - center.y
    const ocz = origin.z - center.z

    const b = ocx * dir.x + ocy * dir.y + ocz * dir.z
    const c = (ocx * ocx + ocy * ocy + ocz * ocz) - radius * radius
    const disc = b * b - c

    return disc >= 0
}

export function raySphereIntersectPoint(origin, dir, center, radius) {
    const ocx = origin.x - center.x
    const ocy = origin.y - center.y
    const ocz = origin.z - center.z

    const b = ocx * dir.x + ocy * dir.y + ocz * dir.z
    const c = (ocx * ocx + ocy * ocy + ocz * ocz) - radius * radius
    const disc = b * b - c

    if (disc < 0) return null

    const sqrtDisc = Math.sqrt(disc)
    const t1 = -b - sqrtDisc
    const t2 = -b + sqrtDisc

    // smallest positive t = nearest hit in front of the ray origin
    let t = t1
    if (t < 0) t = t2
    if (t < 0) return null

    return {
        t: t,
        point: {
            x: origin.x + dir.x * t,
            y: origin.y + dir.y * t,
            z: origin.z + dir.z * t,
        }
    }
}

export function rayPlaneIntersect(origin, dir, planeNormal, planePoint) {
    const denom = dir.x * planeNormal.x + dir.y * planeNormal.y + dir.z * planeNormal.z

    if (Math.abs(denom) < 1e-6) return null

    const diffx = planePoint.x - origin.x
    const diffy = planePoint.y - origin.y
    const diffz = planePoint.z - origin.z

    const t = (diffx * planeNormal.x + diffy * planeNormal.y + diffz * planeNormal.z) / denom

    if (t < 0) return null

    return {
        t: t,
        point: {
            x: origin.x + dir.x * t,
            y: origin.y + dir.y * t,
            z: origin.z + dir.z * t,
        }
    }
}
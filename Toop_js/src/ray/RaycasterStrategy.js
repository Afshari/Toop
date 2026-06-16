export class RaycasterStrategy {

    /**
     * Compute a ray from the camera through a screen-space mouse position.
     * @param {THREE.Vector2} mouseNDC - normalized device coordinates (-1 to 1)
     * @param {THREE.Camera} camera
     * @returns {{ origin: THREE.Vector3, dir: THREE.Vector3 }}
     */
    castRay(mouseNDC, camera) {
        throw new Error('RaycasterStrategy.castRay() not implemented')
    }

    /**
     * Test ray against a sphere, returns nearest hit point in front of origin.
     * @param {{ origin: THREE.Vector3, dir: THREE.Vector3 }} ray
     * @param {THREE.Vector3} center
     * @param {number} radius
     * @returns {{ t: number, point: THREE.Vector3 } | null}
     */
    intersectSphere(ray, center, radius) {
        throw new Error('RaycasterStrategy.intersectSphere() not implemented')
    }

    /**
     * Test ray against an infinite plane defined by normal and a point on the plane.
     * @param {{ origin: THREE.Vector3, dir: THREE.Vector3 }} ray
     * @param {THREE.Vector3} normal
     * @param {THREE.Vector3} point
     * @returns {{ t: number, point: THREE.Vector3 } | null}
     */
    intersectPlane(ray, normal, point) {
        throw new Error('RaycasterStrategy.intersectPlane() not implemented')
    }
}
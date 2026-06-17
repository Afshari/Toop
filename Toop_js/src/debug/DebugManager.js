import * as THREE from 'three'
import { RaySnapshot } from './RaySnapshot.js'

const RAY_LENGTH = 20

export class DebugManager {

    constructor(scene, sphere, raycasterContext) {
        this._scene = scene
        this._sphere = sphere
        this._raycasterContext = raycasterContext

        this._snapshots = []
        this._snapshotLines = []

        this._visible = {
            axes: true,
            rayLine: true,
            dragPlane: true,
            velocity: true,
            snapshots: true,
        }

        this._initAxes()
        this._initRayLine()
        this._initDragPlane()
        this._initVelocityArrow()
    }

    // ------------------------------------------------------------
    // init helpers
    // ------------------------------------------------------------
    _initAxes() {
        this.axesHelper = new THREE.AxesHelper(this._sphere.radius * 2)
        this._scene.add(this.axesHelper)
    }

    _initRayLine() {
        this.rayLine = new THREE.Line(
            new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(),
                new THREE.Vector3()
            ]),
            new THREE.LineBasicMaterial({ color: 0xff00ff })
        )
        this._scene.add(this.rayLine)

        this.rayHitMarker = new THREE.Mesh(
            new THREE.SphereGeometry(0.03, 8, 8),
            new THREE.MeshBasicMaterial({ color: 0x00ffff })
        )
        this.rayHitMarker.visible = false
        this._scene.add(this.rayHitMarker)
    }

    _initDragPlane() {
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
        this._scene.add(this.dragPlaneMesh)

        this._planeAlignQuat = new THREE.Quaternion()
        this._planeDefaultNormal = new THREE.Vector3(0, 0, 1)
    }

    _initVelocityArrow() {
        this.velocityArrow = new THREE.ArrowHelper(
            new THREE.Vector3(0, 1, 0),
            new THREE.Vector3(0, 0, 0),
            0.5,
            0x00ff00
        )
        this._scene.add(this.velocityArrow)
    }

    // ------------------------------------------------------------
    // per-frame update
    // ------------------------------------------------------------
    update(ray, camera) {
        const center = this._sphere.getCenter()
        const orientation = this._sphere.getOrientation()
        const velocity = this._sphere.physics.velocity

        // axes
        this.axesHelper.position.copy(center)
        this.axesHelper.quaternion.copy(orientation)

        // live ray line
        const far = ray.origin.clone().addScaledVector(ray.dir, RAY_LENGTH)
        const positions = this.rayLine.geometry.attributes.position
        positions.setXYZ(0, ray.origin.x, ray.origin.y, ray.origin.z)
        positions.setXYZ(1, far.x, far.y, far.z)
        positions.needsUpdate = true

        // ray hit marker
        const hit = this._raycasterContext.intersectSphere(ray, center, this._sphere.radius)
        if (hit) {
            this.rayHitMarker.position.copy(hit.point)
            this.rayHitMarker.visible = this._visible.rayLine
        } else {
            this.rayHitMarker.visible = false
        }

        // drag plane
        if (this._sphere.isDragging) {
            camera.getWorldDirection(this._planeDefaultNormal)
            this.dragPlaneMesh.position.copy(center)
            this._planeAlignQuat.setFromUnitVectors(
                new THREE.Vector3(0, 0, 1),
                this._planeDefaultNormal
            )
            this.dragPlaneMesh.quaternion.copy(this._planeAlignQuat)
            this.dragPlaneMesh.visible = this._visible.dragPlane
        } else {
            this.dragPlaneMesh.visible = false
        }

        // velocity arrow
        const speed = velocity.length()
        if (speed > 0.001) {
            this.velocityArrow.position.copy(center)
            this.velocityArrow.setDirection(velocity.clone().normalize())
            this.velocityArrow.setLength(Math.min(speed * 0.5, 2.0))
            this.velocityArrow.visible = this._visible.velocity
        } else {
            this.velocityArrow.visible = false
        }
    }

    // ------------------------------------------------------------
    // snapshots
    // ------------------------------------------------------------
    saveSnapshot(ray, camera) {
        const center = this._sphere.getCenter()
        const forward = new THREE.Vector3()
        camera.getWorldDirection(forward)

        // find 3D mouse target point - try sphere first, fall back to front plane
        let mouseTarget = null

        const sphereHit = this._raycasterContext.intersectSphere(ray, center, this._sphere.radius)
        if (sphereHit) {
            mouseTarget = sphereHit.point
        } else {
            const frontPoint = center.clone().addScaledVector(forward, -this._sphere.radius)
            const planeHit = this._raycasterContext.intersectPlane(ray, forward, frontPoint)
            if (planeHit) mouseTarget = planeHit.point
        }

        if (!mouseTarget) return

        const eyes = this._sphere.eyes.getEyes()
        const leftEyeRay = this._buildEyeRay(eyes[0], mouseTarget)
        const rightEyeRay = this._buildEyeRay(eyes[1], mouseTarget)

        // get the actual planes the pupils use for tracking
        const eyePlanes = this._sphere.eyes.getEyePlanes()
        // console.log('eyePlanes', eyePlanes)
        // console.log('left normal', eyePlanes[0].normal)
        // console.log('left point', eyePlanes[0].point)


        const snapshot = new RaySnapshot(
            ray,
            leftEyeRay,
            rightEyeRay,
            eyePlanes[0],
            eyePlanes[1]
        )
        this._snapshots.push(snapshot)
        this._drawSnapshot(snapshot)
    }

    _buildEyeRay(eye, targetPoint) {
        const origin = eye.worldPos.clone()
        const dir = targetPoint.clone().sub(origin).normalize()
        return { origin, dir }
    }
    _drawSnapshot(snapshot) {
        const lines = []

        // camera ray - magenta
        lines.push(this._makeLine(snapshot.cameraRay.origin, snapshot.cameraRay.dir, 0xff00ff))

        // left eye ray - orange
        lines.push(this._makeLine(snapshot.leftEyeRay.origin, snapshot.leftEyeRay.dir, 0xff8800))

        // right eye ray - yellow
        lines.push(this._makeLine(snapshot.rightEyeRay.origin, snapshot.rightEyeRay.dir, 0xffff00))

        // left eye plane - orange tint
        if (snapshot.leftPlane) {
            lines.push(this._makePlaneMesh(snapshot.leftPlane, 0xff8800))
        }

        // right eye plane - yellow tint
        if (snapshot.rightPlane) {
            lines.push(this._makePlaneMesh(snapshot.rightPlane, 0xffff00))
        }

        for (const obj of lines) this._snapshotLines.push(obj)
    }

    _makeLine(origin, dir, color) {
        const far = origin.clone().addScaledVector(dir, RAY_LENGTH)
        const geo = new THREE.BufferGeometry().setFromPoints([origin, far])
        const mat = new THREE.LineBasicMaterial({ color })
        const line = new THREE.Line(geo, mat)
        this._scene.add(line)
        return line
    }

    _makePlaneMesh(plane, color) {
        const defaultNormal = new THREE.Vector3(0, 0, 1)
        if (Math.abs(plane.normal.dot(defaultNormal) + 1) < 1e-6) {
            defaultNormal.set(1, 0, 0)
        }
        const mesh = new THREE.Mesh(
            new THREE.PlaneGeometry(0.5, 0.5),
            new THREE.MeshBasicMaterial({
                color,
                transparent: true,
                opacity: 0.5,
                side: THREE.DoubleSide,
                depthTest: false,
            })
        )
        // push slightly forward along normal so it does not clip inside sphere
        const position = plane.point.clone().addScaledVector(plane.normal, 0.05)
        mesh.position.copy(position)
        const q = new THREE.Quaternion().setFromUnitVectors(defaultNormal, plane.normal)
        mesh.quaternion.copy(q)
        this._scene.add(mesh)
        return mesh
    }

    clearSnapshots() {
        for (const obj of this._snapshotLines) {
            this._scene.remove(obj)
            obj.geometry.dispose()
            obj.material.dispose()
        }
        this._snapshotLines.length = 0
        this._snapshots.length = 0
    }

    // ------------------------------------------------------------
    // visibility toggles
    // ------------------------------------------------------------
    setVisible(name, value) {
        this._visible[name] = value
        switch (name) {
            case 'axes':
                this.axesHelper.visible = value
                break
            case 'rayLine':
                this.rayLine.visible = value
                break
            case 'dragPlane':
                this.dragPlaneMesh.visible = value
                break
            case 'velocity':
                this.velocityArrow.visible = value
                break
            case 'snapshots':
                for (const obj of this._snapshotLines) obj.visible = value
                break
        }
    }

    dispose() {
        this.clearSnapshots()
        this._scene.remove(this.axesHelper)
        this._scene.remove(this.rayLine)
        this._scene.remove(this.rayHitMarker)
        this._scene.remove(this.dragPlaneMesh)
        this._scene.remove(this.velocityArrow)
    }
}
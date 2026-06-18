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
            axes: false,
            rayLine: true,
            dragPlane: true,
            velocity: true,
            snapshots: true,
            orientationCube: true,
            targetCube: true,
            rollingCube: true,
        }

        this._initOrientationHelpers()
        this._initRayLine()
        this._initDragPlane()
        this._initVelocityArrow()
    }

    // ------------------------------------------------------------
    // init helpers
    // ------------------------------------------------------------
    _initOrientationHelpers() {
        // local axes
        this.axesHelper = new THREE.AxesHelper(this._sphere.radius * 2)
        this._scene.add(this.axesHelper)

        // wireframe cube - current orientation
        const cubeSize = this._sphere.radius * 2
        this.orientationCube = new THREE.LineSegments(
            new THREE.EdgesGeometry(new THREE.BoxGeometry(cubeSize, cubeSize, cubeSize)),
            new THREE.LineBasicMaterial({ color: 0x44aaff, transparent: true, opacity: 0.6 })
        )
        this._scene.add(this.orientationCube)

        // wireframe cube - target orientation (where sphere is heading)
        this.targetCube = new THREE.LineSegments(
            new THREE.EdgesGeometry(new THREE.BoxGeometry(cubeSize, cubeSize, cubeSize)),
            new THREE.LineBasicMaterial({ color: 0xff8800, transparent: true, opacity: 0.4 })
        )
        this._scene.add(this.targetCube)

        // wireframe cube - rolling orientation (pure physics)
        this.rollingCube = new THREE.LineSegments(
            new THREE.EdgesGeometry(new THREE.BoxGeometry(cubeSize, cubeSize, cubeSize)),
            new THREE.LineBasicMaterial({ color: 0x44ff44, transparent: true, opacity: 0.4 })
        )
        this._scene.add(this.rollingCube)
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

        // current orientation - blue cube
        this.axesHelper.position.copy(center)
        this.axesHelper.quaternion.copy(orientation)

        this.orientationCube.position.copy(center)
        this.orientationCube.quaternion.copy(orientation)
        this.orientationCube.visible = this._visible.orientationCube

        this.rollingCube.position.copy(center)
        this.rollingCube.quaternion.copy(this._sphere.character.rollingOrientation)
        this.rollingCube.visible = this._visible.rollingCube

        const targetQuat = this._sphere.character.getTargetQuat()
        if (targetQuat) {
            this.targetCube.position.copy(center)
            this.targetCube.quaternion.copy(targetQuat)
        }
        this.targetCube.visible = this._visible.targetCube && !!targetQuat

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

        const eyePlanes = this._sphere.eyes.getEyePlanes()

        const leftHit = this._raycasterContext.intersectPlane(ray, eyePlanes[0].normal, eyePlanes[0].point)
        const rightHit = this._raycasterContext.intersectPlane(ray, eyePlanes[1].normal, eyePlanes[1].point)

        if (!leftHit || !rightHit) return

        const snapshot = new RaySnapshot(ray, leftHit.point, rightHit.point, eyePlanes[0], eyePlanes[1])
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
        lines.push(this._makeMarker(snapshot.leftHitPoint, 0xff8800))

        // right eye ray - yellow
        lines.push(this._makeMarker(snapshot.rightHitPoint, 0xffff00))

        // add these:
        const leftEyePos = this._sphere.eyes.getEyes()[0].worldPos
        const rightEyePos = this._sphere.eyes.getEyes()[1].worldPos

        lines.push(this._makeSegment(leftEyePos, snapshot.leftHitPoint, 0xff8800))
        lines.push(this._makeSegment(rightEyePos, snapshot.rightHitPoint, 0xffff00))

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

    _makeMarker(point, color) {
        const mesh = new THREE.Mesh(
            new THREE.SphereGeometry(0.03, 8, 8),
            new THREE.MeshBasicMaterial({ color, depthTest: false })
        )
        mesh.position.copy(point)
        this._scene.add(mesh)
        return mesh
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

    _makeSegment(from, to, color) {
        const geo = new THREE.BufferGeometry().setFromPoints([from.clone(), to.clone()])
        const mat = new THREE.LineBasicMaterial({ color })
        const line = new THREE.Line(geo, mat)
        this._scene.add(line)
        return line
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
            case 'orientationCube':
                this.orientationCube.visible = value
                break
            case 'targetCube':
                this.targetCube.visible = value
                break
            case 'rollingCube':
                this.rollingCube.visible = value
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
export class RaySnapshot {

    constructor(cameraRay, leftHitPoint, rightHitPoint, leftPlane, rightPlane) {
        this.cameraRay = { origin: cameraRay.origin.clone(), dir: cameraRay.dir.clone() }
        this.leftHitPoint = leftHitPoint.clone()
        this.rightHitPoint = rightHitPoint.clone()
        this.leftPlane = { normal: leftPlane.normal.clone(), point: leftPlane.point.clone() }
        this.rightPlane = { normal: rightPlane.normal.clone(), point: rightPlane.point.clone() }
    }
}
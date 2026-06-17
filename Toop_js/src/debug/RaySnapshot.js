export class RaySnapshot {

    constructor(cameraRay, leftEyeRay, rightEyeRay, leftPlane, rightPlane) {
        this.cameraRay = {
            origin: cameraRay.origin.clone(),
            dir: cameraRay.dir.clone()
        }
        this.leftEyeRay = {
            origin: leftEyeRay.origin.clone(),
            dir: leftEyeRay.dir.clone()
        }
        this.rightEyeRay = {
            origin: rightEyeRay.origin.clone(),
            dir: rightEyeRay.dir.clone()
        }
        this.leftPlane = leftPlane ? {
            normal: leftPlane.normal.clone(),
            point: leftPlane.point.clone()
        } : null
        this.rightPlane = rightPlane ? {
            normal: rightPlane.normal.clone(),
            point: rightPlane.point.clone()
        } : null
    }
}
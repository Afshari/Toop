export class RaycasterContext {

    constructor(strategy) {
        this._strategy = strategy
    }

    setStrategy(strategy) {
        this._strategy = strategy
    }

    getStrategyName() {
        return this._strategy.constructor.name
    }

    castRay(mouseNDC, camera) {
        return this._strategy.castRay(mouseNDC, camera)
    }

    intersectSphere(ray, center, radius) {
        return this._strategy.intersectSphere(ray, center, radius)
    }

    intersectPlane(ray, normal, point) {
        return this._strategy.intersectPlane(ray, normal, point)
    }
}
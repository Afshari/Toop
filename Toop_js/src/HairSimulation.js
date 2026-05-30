import * as THREE from 'three'
import { GPUComputationRenderer } from 'three/addons/misc/GPUComputationRenderer.js'
import { PARAMS } from './config.js'
import integrateSrc from './shaders/integrate.glsl?raw'
import constraintsSrc from './shaders/solve_constraints.glsl?raw'
import sphereColSrc from './shaders/sphere_collision.glsl?raw'
import groundColSrc from './shaders/ground_collision.glsl?raw'
import updateVelocitySrc from './shaders/update_velocity.glsl?raw'
import translateAllSrc from './shaders/translate_all.glsl?raw'


// copies texPosition (injected via dependency from posVar) to prevPosition
const copyPosToPrevPosSrc = /* glsl */`
uniform sampler2D uOldPosition;
void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    gl_FragColor = texture2D(uOldPosition, uv);
}`

export class HairSimulation {

    constructor(renderer, strandGeometry) {
        this.renderer = renderer
        this.texSize = strandGeometry.getTexSize()      // 296
        this.roots = strandGeometry.getRoots()
        this.numStrands = this.roots.length             // 14549

        this._initTextures()
        this._initCompute()

        this.currentPosTex = this.initPosTex
        this.currentVelTex = this.initVelTex
    }

    // -------------------------------------------------------
    // Build initial Float32Arrays and DataTextures
    // -------------------------------------------------------
    _initTextures() {
        const {
            sphere_center, sphere_radius,
            num_segments, segment_length, masses
        } = PARAMS

        const particlesPerStrand = num_segments + 1   // 6
        const size = this.texSize
        const total = size * size                      // 296*296 = 87616

        const posData = new Float32Array(total * 4)
        const prevPosData = new Float32Array(total * 4)
        const velData = new Float32Array(total * 4)   // stays zero
        const rootOffData = new Float32Array(total * 4)   // stays zero except at roots

        // inv_masses for particles j=1..5  ->  [2.0, 2.0, 2.5, 3.33, 5.0]
        const invMasses = masses.map(m => m > 0.0 ? 1.0 / m : 0.0)

        for (let si = 0; si < this.numStrands; si++) {
            const n = this.roots[si]

            // root world position
            const rx = sphere_center[0] + n.x * sphere_radius
            const ry = sphere_center[1] + n.y * sphere_radius
            const rz = sphere_center[2] + n.z * sphere_radius

            for (let j = 0; j < particlesPerStrand; j++) {
                const idx = si * particlesPerStrand + j
                const i4 = idx * 4
                const invM = j === 0 ? 0.0 : invMasses[j - 1]

                // initial position: hang straight down from root
                posData[i4] = PARAMS.sphere_center[0] + n.x * (PARAMS.sphere_radius + j * PARAMS.segment_length)
                posData[i4 + 1] = PARAMS.sphere_center[1] + n.y * (PARAMS.sphere_radius + j * PARAMS.segment_length)
                posData[i4 + 2] = PARAMS.sphere_center[2] + n.z * (PARAMS.sphere_radius + j * PARAMS.segment_length)
                posData[i4 + 3] = invM      // inv_mass stored in w

                prevPosData[i4] = posData[i4]
                prevPosData[i4 + 1] = posData[i4 + 1]
                prevPosData[i4 + 2] = posData[i4 + 2]
                prevPosData[i4 + 3] = invM

                // root offset: local sphere space (only at j==0)
                if (j === 0) {
                    rootOffData[i4] = n.x * sphere_radius
                    rootOffData[i4 + 1] = n.y * sphere_radius
                    rootOffData[i4 + 2] = n.z * sphere_radius
                    rootOffData[i4 + 3] = 0.0
                }
                // vel stays zero
            }
        }

        const makeTex = (data) => {
            const t = new THREE.DataTexture(
                data, size, size, THREE.RGBAFormat, THREE.FloatType)
            t.needsUpdate = true
            return t
        }

        this.initPosTex = makeTex(posData)
        this.initPrevPosTex = makeTex(prevPosData)
        this.initVelTex = makeTex(velData)
        this.rootOffsetTex = makeTex(rootOffData)   // static - never ping-ponged
    }

    // -------------------------------------------------------
    // One GPUComputationRenderer per pass
    // -------------------------------------------------------
    _initCompute() {
        console.log('integrate src loaded:', typeof integrateSrc, integrateSrc?.slice(0, 40))
        console.log('constraints src loaded:', typeof constraintsSrc, constraintsSrc?.slice(0, 40))
        console.log('sphereCol src loaded:', typeof sphereColSrc, sphereColSrc?.slice(0, 40))
        console.log('groundCol src loaded:', typeof groundColSrc, groundColSrc?.slice(0, 40))
        console.log('velocity src loaded:', typeof updateVelocitySrc, updateVelocitySrc?.slice(0, 40))

        const s = this.texSize
        const r = this.renderer

        // ---- integrate: updates position + saves prevPosition ----
        // Two variables computed simultaneously - both read old position
        this.gpuIntegrate = new GPUComputationRenderer(s, s, r)
        this.posVar = this.gpuIntegrate.addVariable('texPosition', integrateSrc, this.initPosTex)
        this.prevPosVar = this.gpuIntegrate.addVariable(
            'texPrevPosition', copyPosToPrevPosSrc, this.initPrevPosTex)
        // prevPosVar depends on posVar so texPosition is injected into its shader
        // during compute() texPosition = posVar's OLD texture (before this substep)
        this.gpuIntegrate.setVariableDependencies(this.posVar, [])
        this.gpuIntegrate.setVariableDependencies(this.prevPosVar, [])
        Object.assign(this.prevPosVar.material.uniforms, {
            uOldPosition: { value: this.initPosTex }
        })
        Object.assign(this.posVar.material.uniforms, {
            uPosition: { value: this.initPosTex },
            uVelocity: { value: this.initVelTex },
            uRootOffset: { value: this.rootOffsetTex },
            uSphereCenter: { value: new THREE.Vector3(...PARAMS.sphere_center) },
            uSphereQuat: { value: new THREE.Vector4(0, 0, 0, 1) },
            uGravity: { value: PARAMS.gravity },
            uDt: { value: 0.0 },
            uDamping: { value: PARAMS.damping },
        })
        this._checkError(this.gpuIntegrate.init(), 'gpuIntegrate')

        // ---- constraints: one renderer called 5 times per substep ----
        // constraintIndex 0..4 selects which segment pair to solve
        this.gpuConstraints = new GPUComputationRenderer(s, s, r)
        this.constraintVar = this.gpuConstraints.addVariable('texConstraintPos', constraintsSrc, this.initPosTex)
        this.gpuConstraints.setVariableDependencies(this.constraintVar, [])
        Object.assign(this.constraintVar.material.uniforms, {
            uPosition: { value: this.initPosTex },
            uConstraintIndex: { value: 0 },
            uRestLength: { value: PARAMS.segment_length },
            uNumStrands: { value: this.numStrands },
        })
        this._checkError(this.gpuConstraints.init(), 'gpuConstraints')

        // ---- sphere collision ----
        this.gpuSphereCol = new GPUComputationRenderer(s, s, r)
        this.sphereColVar = this.gpuSphereCol.addVariable(
            'texSphereColPos', sphereColSrc, this.initPosTex)
        this.gpuSphereCol.setVariableDependencies(this.sphereColVar, [])
        Object.assign(this.sphereColVar.material.uniforms, {
            uPosition: { value: this.initPosTex },
            uSphereCenter: { value: new THREE.Vector3(...PARAMS.sphere_center) },
            uSphereRadius: { value: PARAMS.sphere_radius },
        })
        this._checkError(this.gpuSphereCol.init(), 'gpuSphereCol')

        // ---- ground collision ----
        this.gpuGroundCol = new GPUComputationRenderer(s, s, r)
        this.groundColVar = this.gpuGroundCol.addVariable(
            'texGroundColPos', groundColSrc, this.initPosTex)
        this.gpuGroundCol.setVariableDependencies(this.groundColVar, [])
        Object.assign(this.groundColVar.material.uniforms, {
            uPosition: { value: this.initPosTex },
            uGroundY: { value: PARAMS.ground_y },
        })
        this._checkError(this.gpuGroundCol.init(), 'gpuGroundCol')

        // ---- velocity update ----
        this.gpuVelocity = new GPUComputationRenderer(s, s, r)
        this.velVar = this.gpuVelocity.addVariable(
            'texVelocity', updateVelocitySrc, this.initVelTex)
        this.gpuVelocity.setVariableDependencies(this.velVar, [])
        Object.assign(this.velVar.material.uniforms, {
            uPosition: { value: this.initPosTex },
            uPrevPos: { value: this.initPrevPosTex },
            uVelocity: { value: this.initVelTex },
            uDt: { value: 0.0 },
        })
        this._checkError(this.gpuVelocity.init(), 'gpuVelocity')

        // ---- translate all - used when dragging ----
        this.gpuTranslateAll = new GPUComputationRenderer(s, s, r)
        this.translateAllVar = this.gpuTranslateAll.addVariable(
            'texTranslateAllPos', translateAllSrc, this.initPosTex)
        this.gpuTranslateAll.setVariableDependencies(this.translateAllVar, [])
        Object.assign(this.translateAllVar.material.uniforms, {
            uPosition: { value: this.initPosTex },
            uDelta: { value: new THREE.Vector3() },
        })
        this._checkError(this.gpuTranslateAll.init(), 'gpuTranslateAll')
    }

    _checkError(err, name) {
        if (err) console.error(`[HairSimulation] ${name} init error:`, err)
    }

    // -------------------------------------------------------
    // Current position texture - used by the strand vertex shader
    // Points to ground collision output (last position pass)
    // -------------------------------------------------------
    getPositionTexture() {
        return this.gpuGroundCol.getCurrentRenderTarget(this.groundColVar).texture
    }

    // -------------------------------------------------------
    // Simulation update - substep loop
    // Called each frame from main.js
    // -------------------------------------------------------
    update(dt, sphereCenter, sphereQuaternion, isDragging, frameDelta) {
        if (dt <= 0) return

        const subDt = dt / PARAMS.num_substeps
        const center = sphereCenter || new THREE.Vector3(...PARAMS.sphere_center)
        const quat = sphereQuaternion || new THREE.Quaternion()

        let currentPos = this.currentPosTex
        let currentVel = this.currentVelTex

        // --- translate all particles when dragging ---
        if (isDragging && frameDelta && frameDelta.length() > 1e-6) {
            this.translateAllVar.material.uniforms.uPosition.value = currentPos
            this.translateAllVar.material.uniforms.uDelta.value.copy(frameDelta)
            this.gpuTranslateAll.compute()
            currentPos = this.gpuTranslateAll.getCurrentRenderTarget(this.translateAllVar).texture
        }

        for (let step = 0; step < PARAMS.num_substeps; step++) {

            // --- integrate + save prevPos ---
            const iU = this.posVar.material.uniforms
            iU.uPosition.value = currentPos
            iU.uVelocity.value = currentVel
            iU.uSphereCenter.value.copy(center)
            iU.uSphereQuat.value.set(quat.x, quat.y, quat.z, quat.w)
            iU.uDt.value = subDt
            this.prevPosVar.material.uniforms.uOldPosition.value = currentPos
            this.gpuIntegrate.compute()

            const prevPosTex = this.gpuIntegrate.getCurrentRenderTarget(this.prevPosVar).texture
            let posTex = this.gpuIntegrate.getCurrentRenderTarget(this.posVar).texture

            // --- constraints x5 (Gauss-Seidel: each pass reads previous pass output) ---
            for (let ci = 0; ci < PARAMS.num_segments; ci++) {
                this.constraintVar.material.uniforms.uPosition.value = posTex
                this.constraintVar.material.uniforms.uConstraintIndex.value = ci
                this.gpuConstraints.compute()
                posTex = this.gpuConstraints.getCurrentRenderTarget(this.constraintVar).texture
            }

            // --- sphere collision ---
            this.sphereColVar.material.uniforms.uPosition.value = posTex
            this.sphereColVar.material.uniforms.uSphereCenter.value.copy(center)
            this.gpuSphereCol.compute()
            posTex = this.gpuSphereCol.getCurrentRenderTarget(this.sphereColVar).texture

            // --- ground collision ---
            this.groundColVar.material.uniforms.uPosition.value = posTex
            this.gpuGroundCol.compute()
            posTex = this.gpuGroundCol.getCurrentRenderTarget(this.groundColVar).texture

            // --- velocity ---
            const vU = this.velVar.material.uniforms
            vU.uPosition.value = posTex
            vU.uPrevPos.value = prevPosTex
            vU.uDt.value = subDt
            this.gpuVelocity.compute()
            currentVel = this.gpuVelocity.getCurrentRenderTarget(this.velVar).texture

            currentPos = posTex
        }

        this.currentPosTex = currentPos
        this.currentVelTex = currentVel
    }

    dispose() {
        this.rootOffsetTex.dispose()
        this.initPosTex.dispose()
        this.initPrevPosTex.dispose()
        this.initVelTex.dispose()
    }
}
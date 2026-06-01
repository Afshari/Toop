import * as THREE from 'three'
import { PARAMS } from './config.js'

// ------------------------------------------------------------
// Fibonacci sphere - evenly distributed normals on a unit sphere
// ------------------------------------------------------------
function fibonacciSphere(count) {
    const points = []
    const goldenRatio = (1.0 + Math.sqrt(5.0)) / 2.0
    for (let i = 0; i < count; i++) {
        const theta = Math.acos(1.0 - 2.0 * (i + 0.5) / count)
        const phi = 2.0 * Math.PI * i / goldenRatio
        points.push(new THREE.Vector3(
            Math.sin(theta) * Math.cos(phi),
            Math.sin(theta) * Math.sin(phi),
            Math.cos(theta)
        ))
    }
    return points
}

// Returns true if this normal falls inside a bald eye patch
function isEyePatch(normal) {
    return (
        normal.dot(PARAMS.eye_left_dir) > PARAMS.eye_dot_threshold ||
        normal.dot(PARAMS.eye_right_dir) > PARAMS.eye_dot_threshold
    )
}

// Map a flat particle index to a UV in the simulation texture
function particleIndexToUV(index, texSize) {
    const px = index % texSize
    const py = Math.floor(index / texSize)
    return [
        (px + 0.5) / texSize,
        (py + 0.5) / texSize
    ]
}

// ------------------------------------------------------------
// StrandGeometry
// ------------------------------------------------------------
export class StrandGeometry {
    constructor() {
        this.roots = []
        this.numStrands = 0
        this.texSize = 0
        this.mesh = null

        this._build()
    }

    getRoots() { return this.roots }
    getTexSize() { return this.texSize }

    _build() {
        const {
            target_strand_count,
            sphere_radius,
            num_segments,
            segment_length,
        } = PARAMS

        const particlesPerStrand = num_segments + 1  // 6

        // Generate points and remove eye patches
        const allNormals = fibonacciSphere(target_strand_count)
        this.roots = allNormals.filter(n => !isEyePatch(n))
        this.numStrands = this.roots.length

        // One texel per particle - find smallest square that fits
        const totalParticles = this.numStrands * particlesPerStrand
        this.texSize = Math.ceil(Math.sqrt(totalParticles))

        console.log(
            '[StrandGeometry]',
            '\n  target:', target_strand_count,
            '\n  strands after eye filter:', this.numStrands,
            '\n  total particles:', totalParticles,
            '\n  texture:', this.texSize, 'x', this.texSize,
            '(' + (this.texSize * this.texSize) + ' texels)'
        )

        // LineSegments needs 2 vertices per segment, 5 segments per strand
        const vertsPerStrand = num_segments * 2
        const totalVerts = this.numStrands * vertsPerStrand

        const positions = new Float32Array(totalVerts * 3)
        const particleUVs = new Float32Array(totalVerts * 2)

        let vi = 0  // current vertex index

        for (let si = 0; si < this.numStrands; si++) {
            const n = this.roots[si]
            const rootX = PARAMS.sphere_center[0] + n.x * sphere_radius
            const rootY = PARAMS.sphere_center[1] + n.y * sphere_radius
            const rootZ = PARAMS.sphere_center[2] + n.z * sphere_radius

            // random lean matching Python (offset_x, offset_z in [-0.5, 0.5])
            const ox = (Math.random() - 0.5)
            const oz = (Math.random() - 0.5)

            for (let seg = 0; seg < num_segments; seg++) {
                const globalIdxA = si * particlesPerStrand + seg
                const globalIdxB = si * particlesPerStrand + seg + 1

                // particle A world position
                const axW = rootX + seg * segment_length * ox
                const ayW = rootY - seg * segment_length
                const azW = rootZ + seg * segment_length * oz

                // particle B world position
                const bxW = rootX + (seg + 1) * segment_length * ox
                const byW = rootY - (seg + 1) * segment_length
                const bzW = rootZ + (seg + 1) * segment_length * oz

                positions[vi * 3 + 0] = axW
                positions[vi * 3 + 1] = ayW
                positions[vi * 3 + 2] = azW
                const uvA = particleIndexToUV(globalIdxA, this.texSize)
                particleUVs[vi * 2 + 0] = uvA[0]
                particleUVs[vi * 2 + 1] = uvA[1]
                vi++

                positions[vi * 3 + 0] = bxW
                positions[vi * 3 + 1] = byW
                positions[vi * 3 + 2] = bzW
                const uvB = particleIndexToUV(globalIdxB, this.texSize)
                particleUVs[vi * 2 + 0] = uvB[0]
                particleUVs[vi * 2 + 1] = uvB[1]
                vi++
            }
        }

        const geo = new THREE.BufferGeometry()
        geo.setAttribute('position', new THREE.BufferAttribute(positions, 3))
        geo.setAttribute('particleUV', new THREE.BufferAttribute(particleUVs, 2))

        // Simple CPU material - replaced by ShaderMaterial in js/gpgpu-core
        const mat = new THREE.LineBasicMaterial({ color: 0xddccaa })

        this.mesh = new THREE.LineSegments(geo, mat)
    }

    connectSimulation(positionTex) {
        import('./shaders/strand_vert.glsl?raw').then(vertMod =>
            import('./shaders/strand_frag.glsl?raw').then(fragMod => {
                const mat = new THREE.ShaderMaterial({
                    vertexShader: vertMod.default,
                    fragmentShader: fragMod.default,
                    uniforms: {
                        uPositionTex: { value: positionTex },
                        uColor: { value: PARAMS.hair_color },
                        uLightDir: { value: new THREE.Vector3(5, 8, 5).normalize() },
                        uTexSize: { value: this.texSize },
                    },
                    transparent: true,
                    depthWrite:  true,
                })
                this.mesh.material.dispose()
                this.mesh.material = mat
                this.mesh.frustumCulled = false   // bounding box is wrong since positions are in texture
            }))
    }

    dispose() {
        this.mesh.geometry.dispose()
        this.mesh.material.dispose()
    }
}
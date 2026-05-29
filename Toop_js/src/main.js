import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { PARAMS } from './config.js'
import { StrandGeometry } from './StrandGeometry.js'
import { HairSimulation } from './HairSimulation.js'

// ------------------------------------------------------------
// Renderer
// ------------------------------------------------------------
const renderer = new THREE.WebGLRenderer({ antialias: true })
renderer.setSize(window.innerWidth, window.innerHeight)
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
renderer.shadowMap.enabled = true
document.body.appendChild(renderer.domElement)

// ------------------------------------------------------------
// Scene
// ------------------------------------------------------------
const scene = new THREE.Scene()
scene.background = new THREE.Color(0x111111)

// ------------------------------------------------------------
// Camera
// ------------------------------------------------------------
const camera = new THREE.PerspectiveCamera(
    45,
    window.innerWidth / window.innerHeight,
    0.1,
    100
)
camera.position.set(0, 1.5, 4)

// ------------------------------------------------------------
// Controls
// ------------------------------------------------------------
const controls = new OrbitControls(camera, renderer.domElement)
controls.enableDamping = true
controls.dampingFactor = 0.05
controls.target.set(0, 1.5, 0)

// ------------------------------------------------------------
// Lighting
// ------------------------------------------------------------
const ambientLight = new THREE.AmbientLight(0xffffff, 0.4)
scene.add(ambientLight)

const dirLight = new THREE.DirectionalLight(0xffffff, 1.0)
dirLight.position.set(5, 8, 5)
dirLight.castShadow = true
scene.add(dirLight)

// ------------------------------------------------------------
// Sphere - placeholder, replaced by Sphere class in js/sphere-physics
// ------------------------------------------------------------
const sphereGeo = new THREE.SphereGeometry(PARAMS.sphere_radius, 32, 32)
const sphereMat = new THREE.MeshStandardMaterial({ color: 0x555555 })
const sphereMesh = new THREE.Mesh(sphereGeo, sphereMat)
sphereMesh.castShadow = true
sphereMesh.position.set(
    PARAMS.sphere_center[0],
    PARAMS.sphere_center[1],
    PARAMS.sphere_center[2]
)
scene.add(sphereMesh)

// ------------------------------------------------------------
// Ground
// ------------------------------------------------------------
const groundGeo = new THREE.PlaneGeometry(20, 20)
const groundMat = new THREE.MeshStandardMaterial({ color: 0x333333 })
const ground = new THREE.Mesh(groundGeo, groundMat)
ground.rotation.x = -Math.PI / 2
ground.position.y = PARAMS.ground_y
ground.receiveShadow = true
scene.add(ground)

// ------------------------------------------------------------
// Strand geometry
// ------------------------------------------------------------
const strandGeometry = new StrandGeometry()
scene.add(strandGeometry.mesh)
const simulation = new HairSimulation(renderer, strandGeometry)
strandGeometry.connectSimulation(simulation.getPositionTexture())

// ------------------------------------------------------------
// Resize handler
// ------------------------------------------------------------
window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight
    camera.updateProjectionMatrix()
    renderer.setSize(window.innerWidth, window.innerHeight)
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
})

// ------------------------------------------------------------
// Animation loop
// ------------------------------------------------------------
function animate() {
    requestAnimationFrame(animate)
    controls.update()

    simulation.update(1 / 60, null, null)

    if (strandGeometry.mesh.material.uniforms) {
        strandGeometry.mesh.material.uniforms.uPositionTex.value =
            simulation.getPositionTexture()
    }

    renderer.render(scene, camera)
}

animate()
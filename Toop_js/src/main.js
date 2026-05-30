import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { PARAMS } from './config.js'
import { StrandGeometry } from './StrandGeometry.js'
import { HairSimulation } from './HairSimulation.js'
import { Sphere } from './Sphere.js'
const clock = new THREE.Clock()

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

const mouseNDC = new THREE.Vector2(0, 0)
window.addEventListener('mousemove', (e) => {
    mouseNDC.x =  (e.clientX / window.innerWidth)  * 2 - 1
    mouseNDC.y = -(e.clientY / window.innerHeight) * 2 + 1

    if (sphere.isDragging) {
        dragRaycaster.setFromCamera(mouseNDC, camera)
        sphere.handleDragMove(dragRaycaster, camera, 1 / 60)
    }
})

const dragRaycaster = new THREE.Raycaster()

window.addEventListener('mousedown', (e) => {
    dragRaycaster.setFromCamera(mouseNDC, camera)
    const grabbed = sphere.handleDragStart(dragRaycaster)
    if (grabbed) controls.enabled = false
})

window.addEventListener('mouseup', () => {
    if (sphere.isDragging) {
        sphere.handleDragEnd()
        controls.enabled = true
    }
})

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
// Sphere
// ------------------------------------------------------------
const sphere = new Sphere(scene)

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

const roomBox = new THREE.Box3(
    new THREE.Vector3(...PARAMS.room_min),
    new THREE.Vector3(...PARAMS.room_max)
)
const roomHelper = new THREE.Box3Helper(roomBox, 0x444444)
scene.add(roomHelper)

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

    const dt = Math.min(Math.max(clock.getDelta(), 1 / 120), 0.05)

    sphere.update(dt)
    sphere.updateIdleTimer(dt)
    if(!sphere.isDragging) {
        sphere.updateOrientation(dt)
        sphere.rotateTowardCamera(camera.position)
        // sphere.updateHeadTilt(camera, mouseNDC)
    }

    simulation.update(dt, sphere.getCenter(), sphere.getOrientation(),
        sphere.isDragging, sphere.getFrameDelta(), clock.getElapsedTime())

    if (strandGeometry.mesh.material.uniforms) {
        strandGeometry.mesh.material.uniforms.uPositionTex.value =
            simulation.getPositionTexture()
    }

    renderer.render(scene, camera)
}

animate()
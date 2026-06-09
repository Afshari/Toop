import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { PARAMS } from './config.js'
import { StrandGeometry } from './StrandGeometry.js'
import { HairSimulation } from './HairSimulation.js'
import { Sphere } from './Sphere.js'
import Stats from 'stats.js'
import GUI from 'lil-gui'

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
// Stats
// ------------------------------------------------------------
const stats = new Stats()
stats.showPanel(0)
document.body.appendChild(stats.dom)

// ------------------------------------------------------------
// Scene
// ------------------------------------------------------------
const scene = new THREE.Scene()
scene.background = new THREE.Color(0x1a1410)
scene.fog = new THREE.Fog(0x1a1410, 8, 20)

// ------------------------------------------------------------
// Camera
// ------------------------------------------------------------
const camera = new THREE.PerspectiveCamera(
    45,
    window.innerWidth / window.innerHeight,
    0.1,
    100
)
camera.position.set(0, 1.0, 6)

// ------------------------------------------------------------
// Controls
// ------------------------------------------------------------
const controls = new OrbitControls(camera, renderer.domElement)
controls.enableDamping = true
controls.dampingFactor = 0.05
controls.target.set(0, 1.0, 0)

// ------------------------------------------------------------
// Mouse
// ------------------------------------------------------------
const mouseNDC = new THREE.Vector2(0, 0)
window.addEventListener('mousemove', (e) => {
    mouseNDC.x = (e.clientX / window.innerWidth) * 2 - 1
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
// Keyboard
// ------------------------------------------------------------
function setCameraPreset(distance) {
    const spherePos = sphere.getCenter()
    const dir = camera.position.clone().sub(spherePos).normalize()
    camera.position.copy(spherePos).addScaledVector(dir, distance)
    controls.target.copy(spherePos)
    controls.update()
}

window.addEventListener('keydown', (e) => {
    switch (e.key) {
        case '1': setCameraPreset(3.0); break
        case '2': setCameraPreset(4.0); break
        case '3': setCameraPreset(6.0); break
        case '4': setCameraPreset(8.0); break
        case '5': setCameraPreset(12.0); break
        case '6': setCameraPreset(18.0); break
        case 'f':
        case 'F':
            stats.dom.style.display =
                stats.dom.style.display === 'none' ? 'block' : 'none'
            break
        case 'l':
        case 'L':
            ambientLight.intensity = Math.min(3.0, ambientLight.intensity + 0.1)
            dirLight.intensity = Math.min(4.0, dirLight.intensity + 0.1)
            break
        case 'k':
        case 'K':
            ambientLight.intensity = Math.max(0.0, ambientLight.intensity - 0.1)
            dirLight.intensity = Math.max(0.0, dirLight.intensity - 0.1)
            break
    }
})


// ------------------------------------------------------------
// Touch
// ------------------------------------------------------------
window.addEventListener('touchstart', (e) => {
    const touch = e.touches[0]
    mouseNDC.x = (touch.clientX / window.innerWidth) * 2 - 1
    mouseNDC.y = -(touch.clientY / window.innerHeight) * 2 + 1
    dragRaycaster.setFromCamera(mouseNDC, camera)
    const grabbed = sphere.handleDragStart(dragRaycaster)
    if (grabbed) controls.enabled = false
    e.preventDefault()
}, { passive: false })

window.addEventListener('touchmove', (e) => {
    const touch = e.touches[0]
    mouseNDC.x = (touch.clientX / window.innerWidth) * 2 - 1
    mouseNDC.y = -(touch.clientY / window.innerHeight) * 2 + 1
    if (sphere.isDragging) {
        dragRaycaster.setFromCamera(mouseNDC, camera)
        sphere.handleDragMove(dragRaycaster, camera, 1 / 60)
    }
    e.preventDefault()
}, { passive: false })

window.addEventListener('touchend', () => {
    const sphereNDC = sphere.getCenter().clone().project(camera)
    mouseNDC.set(sphereNDC.x, sphereNDC.y)

    if (sphere.isDragging) {
        sphere.handleDragEnd()
        controls.enabled = true
    }
    // mouseNDC.set(0, 0)
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

const fillLight = new THREE.DirectionalLight(0x334466, 0.3)
fillLight.position.set(-5, 2, -5)
scene.add(fillLight)

// ------------------------------------------------------------
// Sphere
// ------------------------------------------------------------
const sphere = new Sphere(scene)

// ------------------------------------------------------------
// Ground
// ------------------------------------------------------------
const groundGeo = new THREE.PlaneGeometry(20, 20)
const groundMat = new THREE.MeshStandardMaterial({
    color: 0x2a2018,
    roughness: 0.8,
})
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
// GUI
// ------------------------------------------------------------
const gui = new GUI()
gui.close()

const simFolder = gui.addFolder('Simulation')
simFolder.add(PARAMS, 'wind_strength', 0, 1, 0.01)
simFolder.add(PARAMS, 'wind_frequency', 0, 2, 0.1)
simFolder.add(PARAMS, 'damping', 0.98, 1.0, 0.001)
simFolder.add(PARAMS, 'sphere_collision_compliance', 0, 0.01, 0.0001)

const envFolder = gui.addFolder('Environment')
// const envParams = { hairColor: '#ddccaa' }
envFolder.addColor(PARAMS, 'hair_color').onChange(v => {
    strandGeometry.mesh.material.uniforms.uColor.value.set(v)
})
envFolder.addColor(PARAMS, 'sphere_color').onChange(v => {
    sphere.mesh.material.color.set(v)
})

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
    stats.begin()

    controls.update()

    const dt = Math.min(Math.max(clock.getDelta(), 1 / 120), 0.05)

    sphere.update(dt)
    sphere.updateIdleTimer(dt)
    if (!sphere.isDragging) {
        sphere.updateOrientation(dt)
        sphere.rotateTowardCamera(camera.position)
        sphere.updateHeadTilt(camera, mouseNDC)
    }
    sphere.updateEyes(camera, mouseNDC)

    simulation.update(dt, sphere.getCenter(), sphere.getOrientation(),
        sphere.isDragging, sphere.getFrameDelta(), clock.getElapsedTime())

    if (strandGeometry.mesh.material.uniforms) {
        strandGeometry.mesh.material.uniforms.uPositionTex.value =
            simulation.getPositionTexture()
    }

    renderer.render(scene, camera)
    stats.end()
}

animate()
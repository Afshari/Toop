import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { PARAMS } from './config.js'
import { StrandGeometry } from './StrandGeometry.js'
import { HairSimulation } from './HairSimulation.js'
import { Sphere } from './sphere/Sphere.js'
import { CustomRaycaster } from './ray/CustomRaycaster.js'
import { ThreeRaycaster } from './ray/ThreeRaycaster.js'
import { RaycasterContext } from './ray/RaycasterContext.js'
import Stats from 'stats.js'
import GUI from 'lil-gui'
import { inject } from '@vercel/analytics'
import { DebugManager } from './debug/DebugManager.js'

inject()

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
// Help
// ------------------------------------------------------------
const helpOverlay = document.getElementById('help-overlay')
helpOverlay.style.display = 'none'

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
// Raycaster
// ------------------------------------------------------------
const customRaycaster = new CustomRaycaster()
const threeRaycaster = new ThreeRaycaster()
const raycasterContext = new RaycasterContext(customRaycaster)

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
const sphere = new Sphere(scene, raycasterContext)
threeRaycaster.setSphereMesh(sphere.mesh)

// ------------------------------------------------------------
// Debug
// ------------------------------------------------------------
const debugManager = new DebugManager(scene, sphere, raycasterContext)

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
// Mouse
// ------------------------------------------------------------
const mouseNDC = new THREE.Vector2(0, 0)

window.addEventListener('mousemove', (e) => {
    mouseNDC.x = (e.clientX / window.innerWidth) * 2 - 1
    mouseNDC.y = -(e.clientY / window.innerHeight) * 2 + 1

    if (sphere.isDragging) {
        const ray = raycasterContext.castRay(mouseNDC, camera)
        sphere.handleDragMove(ray, camera)
    }
})

window.addEventListener('mousedown', () => {
    const ray = raycasterContext.castRay(mouseNDC, camera)
    const grabbed = sphere.handleDragStart(ray)
    if (grabbed) controls.enabled = false
})

window.addEventListener('mouseup', () => {
    if (sphere.isDragging) {
        sphere.handleDragEnd()
        controls.enabled = true
    }
})

// ------------------------------------------------------------
// Touch
// ------------------------------------------------------------
window.addEventListener('touchstart', (e) => {
    const touch = e.touches[0]
    mouseNDC.x = (touch.clientX / window.innerWidth) * 2 - 1
    mouseNDC.y = -(touch.clientY / window.innerHeight) * 2 + 1
    const ray = raycasterContext.castRay(mouseNDC, camera)
    const grabbed = sphere.handleDragStart(ray)
    if (grabbed) controls.enabled = false
    e.preventDefault()
}, { passive: false })

window.addEventListener('touchmove', (e) => {
    const touch = e.touches[0]
    mouseNDC.x = (touch.clientX / window.innerWidth) * 2 - 1
    mouseNDC.y = -(touch.clientY / window.innerHeight) * 2 + 1
    if (sphere.isDragging) {
        const ray = raycasterContext.castRay(mouseNDC, camera)
        sphere.handleDragMove(ray, camera)
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
        case 'r':
        case 'R':
            debugManager.saveSnapshot(raycasterContext.castRay(mouseNDC, camera), camera)
            break
        case 'c':
        case 'C':
            debugManager.clearSnapshots()
            break
        case 'q':
        case 'Q':
            sphere.frozen = !sphere.frozen
            debugParams.freezeSphere = sphere.frozen
            freezeController.updateDisplay()
            break
        case 'o':
        case 'O':
            controls.enabled = !controls.enabled
            debugParams.orbitCamera = controls.enabled
            orbitController.updateDisplay()
            break
        case 'h':
        case 'H':
            helpOverlay.style.display =
                helpOverlay.style.display === 'none' ? 'block' : 'none'
            break
        case 'd':
        case 'D':
            debugFolder._closed ? debugFolder.open() : debugFolder.close()
            break
        case 'a':
        case 'A':
            debugParams.showAxes = !debugParams.showAxes
            debugManager.setVisible('axes', debugParams.showAxes)
            axesController.updateDisplay()
            break
        case 'x':
        case 'X':
            debugParams.showRay = !debugParams.showRay
            debugManager.setVisible('rayLine', debugParams.showRay)
            rayController.updateDisplay()
            break
        case 'z':
        case 'Z':
            debugParams.showDragPlane = !debugParams.showDragPlane
            debugManager.setVisible('dragPlane', debugParams.showDragPlane)
            planeController.updateDisplay()
            break
        case 'v':
        case 'V':
            debugParams.useCustomRaycaster = !debugParams.useCustomRaycaster
            raycasterContext.setStrategy(debugParams.useCustomRaycaster ? customRaycaster : threeRaycaster)
            raycasterController.updateDisplay()
            break
    }
})

// ------------------------------------------------------------
// GUI
// ------------------------------------------------------------
const gui = new GUI()

const simFolder = gui.addFolder('Simulation')
simFolder.close()
simFolder.add(PARAMS, 'wind_strength', 0, 1, 0.01)
simFolder.add(PARAMS, 'wind_frequency', 0, 2, 0.1)
simFolder.add(PARAMS, 'damping', 0.98, 1.0, 0.001)
simFolder.add(PARAMS, 'sphere_collision_compliance', 0, 0.01, 0.0001)

const envFolder = gui.addFolder('Environment')
envFolder.close()
envFolder.addColor(PARAMS, 'hair_color').onChange(v => {
    strandGeometry.mesh.material.uniforms.uColor.value.set(v)
})
envFolder.addColor(PARAMS, 'sphere_color').onChange(v => {
    sphere.mesh.material.color.set(v)
})

const debugParams = {
    showAxes: false,
    showRay: true,
    showDragPlane: true,
    showSnapshots: true,
    useCustomRaycaster: true,
    freezeSphere: false,
    orbitCamera: true,
}

const debugFolder = gui.addFolder('Debug')
debugFolder.close()

const axesController = debugFolder.add(debugParams, 'showAxes').name('Local Axes (A)').onChange(v => {
    debugManager.setVisible('axes', v)
})
const rayController = debugFolder.add(debugParams, 'showRay').name('Camera Ray (X)').onChange(v => {
    debugManager.setVisible('rayLine', v)
})
const planeController = debugFolder.add(debugParams, 'showDragPlane').name('Drag Plane (Z)').onChange(v => {
    debugManager.setVisible('dragPlane', v)
})
const snapshotsController = debugFolder.add(debugParams, 'showSnapshots').name('Snapshots (R/C)').onChange(v => {
    debugManager.setVisible('snapshots', v)
})
const raycasterController = debugFolder.add(debugParams, 'useCustomRaycaster').name('Custom Raycaster (V)').onChange(v => {
    raycasterContext.setStrategy(v ? customRaycaster : threeRaycaster)
})
const freezeController = debugFolder.add(debugParams, 'freezeSphere').name('Freeze Sphere (Q)').onChange(v => {
    sphere.frozen = v
})
const orbitController = debugFolder.add(debugParams, 'orbitCamera').name('Orbit Camera (O)').onChange(v => {
    controls.enabled = v
})

// ------------------------------------------------------------
// Resize
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
const clock = new THREE.Clock()

function animate() {
    requestAnimationFrame(animate)
    stats.begin()

    controls.update()

    const dt = Math.min(Math.max(clock.getDelta(), 1 / 120), 0.05)

    sphere.update(dt)
    sphere.updateIdleTimer(dt)

    if (!sphere.isDragging && !sphere.frozen) {
        sphere.updateOrientation(dt)
        sphere.rotateTowardCamera(camera.position)
        sphere.updateHeadTilt(mouseNDC)
    }

    if (!sphere.frozen)
        sphere.updateEyes(mouseNDC, camera)

    const ray = raycasterContext.castRay(mouseNDC, camera)
    debugManager.update(ray, camera)

    simulation.update(
        dt,
        sphere.getCenter(),
        sphere.getOrientation(),
        sphere.isDragging,
        sphere.getFrameDelta(),
        clock.getElapsedTime()
    )

    if (strandGeometry.mesh.material.uniforms) {
        strandGeometry.mesh.material.uniforms.uPositionTex.value =
            simulation.getPositionTexture()
    }

    renderer.render(scene, camera)
    stats.end()
}

animate()
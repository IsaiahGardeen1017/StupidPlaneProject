import * as THREE from 'https://unpkg.com/three@0.165.0/build/three.module.js';

const STORAGE_KEY = 'stupid-plane-preset-v1';
const dt = 1 / 60;

const tuning = {
  thrust: 40,
  drag: 0.03,
  lift: 0.5,
  rollRate: 1.4,
  pitchRate: 1.4,
};

const control = {
  throttle: 0.6,
  pitch: 0,
  roll: 0,
  trigger: 0,
};

const plane = {
  pos: new THREE.Vector3(0, 120, 0),
  vel: new THREE.Vector3(70, 0, 0),
  pitch: 0,
  yaw: 0,
  roll: 0,
  pitchRate: 0,
  rollRate: 0,
  yawRate: 0,
  health: 100,
  ammo: 250,
  cooldown: 0,
};

const projectiles = [];

const canvas = document.getElementById('scene');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(window.innerWidth, window.innerHeight);

const scene = new THREE.Scene();
scene.background = new THREE.Color('#73a7d8');
scene.fog = new THREE.Fog('#73a7d8', 500, 4000);

const camera = new THREE.PerspectiveCamera(70, window.innerWidth / window.innerHeight, 0.1, 7000);

const sun = new THREE.DirectionalLight(0xffffff, 1.1);
sun.position.set(400, 900, 600);
scene.add(sun);
scene.add(new THREE.AmbientLight(0xbfd8ff, 0.5));

const ground = new THREE.Mesh(
  new THREE.PlaneGeometry(10000, 10000, 8, 8),
  new THREE.MeshStandardMaterial({ color: '#315d2f' })
);
ground.rotation.x = -Math.PI / 2;
scene.add(ground);

const grid = new THREE.GridHelper(10000, 200, 0x88aaff, 0x335577);
grid.position.y = 0.1;
scene.add(grid);

const planeMesh = new THREE.Group();
const fuselage = new THREE.Mesh(
  new THREE.BoxGeometry(5, 1, 1),
  new THREE.MeshStandardMaterial({ color: '#e6e6e6' })
);
const wing = new THREE.Mesh(
  new THREE.BoxGeometry(1.3, 0.15, 8),
  new THREE.MeshStandardMaterial({ color: '#cc3333' })
);
const tail = new THREE.Mesh(
  new THREE.BoxGeometry(1, 1.2, 0.2),
  new THREE.MeshStandardMaterial({ color: '#bbbbbb' })
);
tail.position.set(-2.3, 0.5, 0);
planeMesh.add(fuselage, wing, tail);
scene.add(planeMesh);

const bulletGeo = new THREE.SphereGeometry(0.2, 8, 8);
const bulletMat = new THREE.MeshStandardMaterial({ color: '#ffe68a', emissive: '#664400' });

const telemetryEl = document.getElementById('telemetry');
const presetTextEl = document.getElementById('presetText');

const binds = {
  thrust: document.getElementById('thrust'),
  drag: document.getElementById('drag'),
  lift: document.getElementById('lift'),
  rollRate: document.getElementById('rollRate'),
  pitchRate: document.getElementById('pitchRate'),
};
const bindValues = {
  thrust: document.getElementById('thrustValue'),
  drag: document.getElementById('dragValue'),
  lift: document.getElementById('liftValue'),
  rollRate: document.getElementById('rollRateValue'),
  pitchRate: document.getElementById('pitchRateValue'),
};

function formatTuneValue(key, value) {
  if (key === 'thrust') return value.toFixed(0);
  if (key === 'drag') return value.toFixed(3);
  return value.toFixed(2);
}

for (const [key, el] of Object.entries(binds)) {
  el.value = tuning[key];
  el.addEventListener('input', () => {
    tuning[key] = Number(el.value);
    bindValues[key].textContent = formatTuneValue(key, tuning[key]);
  });
  bindValues[key].textContent = formatTuneValue(key, tuning[key]);
}

function clamp(v, lo, hi) {
  return Math.min(hi, Math.max(lo, v));
}

function savePreset() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(tuning));
}

function loadPreset() {
  const raw = localStorage.getItem(STORAGE_KEY);
  if (!raw) return;
  const next = JSON.parse(raw);
  applyPreset(next);
}

function applyPreset(next) {
  if (!next || typeof next !== 'object') return;
  for (const key of Object.keys(tuning)) {
    if (typeof next[key] === 'number') {
      tuning[key] = next[key];
      binds[key].value = next[key];
      bindValues[key].textContent = formatTuneValue(key, next[key]);
    }
  }
}

document.getElementById('savePreset').addEventListener('click', savePreset);
document.getElementById('loadPreset').addEventListener('click', loadPreset);
document.getElementById('exportPreset').addEventListener('click', () => {
  presetTextEl.value = JSON.stringify(tuning, null, 2);
});
document.getElementById('importPreset').addEventListener('click', () => {
  try {
    const parsed = JSON.parse(presetTextEl.value);
    applyPreset(parsed);
  } catch {
    // Ignore parse errors.
  }
});

const keys = new Set();
window.addEventListener('keydown', (ev) => {
  keys.add(ev.code);
  if (ev.code === 'KeyR') resetFlight();
});
window.addEventListener('keyup', (ev) => {
  keys.delete(ev.code);
});

function resetFlight() {
  plane.pos.set(0, 120, 0);
  plane.vel.set(70, 0, 0);
  plane.pitch = 0;
  plane.roll = 0;
  plane.yaw = 0;
  plane.health = 100;
  plane.ammo = 250;
  plane.cooldown = 0;
  for (const p of projectiles) scene.remove(p.mesh);
  projectiles.length = 0;
}

function spawnProjectile() {
  if (plane.ammo <= 0 || plane.cooldown > 0) return;

  const mesh = new THREE.Mesh(bulletGeo, bulletMat);
  mesh.position.copy(plane.pos);
  scene.add(mesh);

  const orientation = getOrientationQuaternion();
  const forward = new THREE.Vector3(1, 0, 0).applyQuaternion(orientation).normalize();

  projectiles.push({
    mesh,
    vel: forward.multiplyScalar(320).add(plane.vel.clone()),
    ttl: 3,
  });

  plane.cooldown = 0.12;
  plane.ammo -= 1;
}

function getOrientationQuaternion() {
  return new THREE.Quaternion().setFromEuler(new THREE.Euler(plane.roll, plane.yaw, plane.pitch, 'XYZ'));
}

function updateControls() {
  control.pitch = 0;
  control.roll = 0;
  control.trigger = keys.has('Space') ? 1 : 0;

  if (keys.has('KeyW')) control.throttle += 0.4 * dt;
  if (keys.has('KeyS')) control.throttle -= 0.4 * dt;
  control.throttle = clamp(control.throttle, 0, 1);

  if (keys.has('ArrowUp')) control.pitch += 1;
  if (keys.has('ArrowDown')) control.pitch -= 1;
  if (keys.has('ArrowLeft')) control.roll -= 1;
  if (keys.has('ArrowRight')) control.roll += 1;
}

function updateFlight() {
  const speed = plane.vel.length();
  plane.pitchRate = control.pitch * tuning.pitchRate;
  plane.rollRate = control.roll * tuning.rollRate;
  plane.pitch += plane.pitchRate * dt;
  plane.roll += plane.rollRate * dt;

  plane.pitch = clamp(plane.pitch, -1.2, 1.2);
  plane.roll = clamp(plane.roll, -1.4, 1.4);

  plane.yawRate = Math.tan(plane.roll) * (speed / 130) * 0.35;
  plane.yaw += plane.yawRate * dt;

  const orientation = getOrientationQuaternion();
  const forward = new THREE.Vector3(1, 0, 0).applyQuaternion(orientation);
  const wingUp = new THREE.Vector3(0, 1, 0).applyQuaternion(orientation);

  const thrust = forward.clone().multiplyScalar(tuning.thrust * control.throttle);
  const dragDirection = speed > 0.001 ? plane.vel.clone().normalize() : new THREE.Vector3(0, 0, 0);
  const drag = dragDirection.multiplyScalar(-tuning.drag * speed * speed * 0.02);
  const liftScale = speed < 25 ? speed / 25 : 1;
  const lift = wingUp.multiplyScalar(tuning.lift * speed * speed * 0.004 * liftScale);
  const gravity = new THREE.Vector3(0, -9.81, 0);

  const accel = thrust.add(drag).add(lift).add(gravity);
  plane.vel.addScaledVector(accel, dt);
  plane.pos.addScaledVector(plane.vel, dt);

  if (plane.pos.y < 1) {
    plane.pos.y = 1;
    plane.vel.y = Math.max(0, plane.vel.y * -0.2);
  }

  if (plane.cooldown > 0) plane.cooldown -= dt;
  if (control.trigger) spawnProjectile();
}

function updateProjectiles() {
  for (let i = projectiles.length - 1; i >= 0; i--) {
    const p = projectiles[i];
    p.mesh.position.addScaledVector(p.vel, dt);
    p.ttl -= dt;
    if (p.ttl <= 0) {
      scene.remove(p.mesh);
      projectiles.splice(i, 1);
    }
  }
}

function updateCamera() {
  const behind = new THREE.Vector3(-26, 10, 0).applyQuaternion(getOrientationQuaternion());
  camera.position.copy(plane.pos).add(behind);
  camera.lookAt(plane.pos);
}

function updateVisuals() {
  planeMesh.position.copy(plane.pos);
  planeMesh.rotation.set(plane.roll, plane.yaw, plane.pitch, 'XYZ');
}

function computeAoaDegrees() {
  const speed = plane.vel.length();
  if (speed < 0.001) return 0;
  const invOrientation = getOrientationQuaternion().invert();
  const localVelocity = plane.vel.clone().normalize().applyQuaternion(invOrientation);
  const aoaRad = Math.atan2(localVelocity.y, Math.max(0.001, localVelocity.x));
  return THREE.MathUtils.radToDeg(aoaRad);
}

function renderTelemetry() {
  const speed = plane.vel.length();
  const telemetry = {
    speed: speed.toFixed(1),
    altitude: plane.pos.y.toFixed(1),
    climbRate: plane.vel.y.toFixed(2),
    throttle: control.throttle.toFixed(2),
    pitch: THREE.MathUtils.radToDeg(plane.pitch).toFixed(1),
    roll: THREE.MathUtils.radToDeg(plane.roll).toFixed(1),
    yaw: THREE.MathUtils.radToDeg(plane.yaw).toFixed(1),
    rollRate: THREE.MathUtils.radToDeg(plane.rollRate).toFixed(2),
    turnRate: THREE.MathUtils.radToDeg(plane.yawRate).toFixed(2),
    aoaEstimate: computeAoaDegrees().toFixed(2),
    ammo: plane.ammo,
    bullets: projectiles.length,
  };

  telemetryEl.textContent = Object.entries(telemetry)
    .map(([k, v]) => `${k.padEnd(12)}: ${v}`)
    .join('\n');
}

function frame() {
  updateControls();
  updateFlight();
  updateProjectiles();
  updateVisuals();
  updateCamera();
  renderTelemetry();
  renderer.render(scene, camera);
  requestAnimationFrame(frame);
}

window.addEventListener('resize', () => {
  renderer.setSize(window.innerWidth, window.innerHeight);
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
});

resetFlight();
loadPreset();
frame();

import { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';
import { loadSimBridge } from './simBridge';

const DT_MS = 1000 / 60;

function fmt(v, digits = 2) {
  return Number.isFinite(v) ? v.toFixed(digits) : '0.00';
}

export default function App() {
  const mountRef = useRef(null);
  const planeRef = useRef(null);
  const simRef = useRef(null);
  const keysRef = useRef(new Set());
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');
  const [telemetry, setTelemetry] = useState({
    speed: 0,
    altitude: 0,
    climbRate: 0,
    pitchAttitude: 0,
    rollAttitude: 0,
    turnRate: 0,
    aoaEstimate: 0,
    throttle: 0,
    health: 0,
    ammo: 0,
  });

  useEffect(() => {
    const mount = mountRef.current;
    const scene = new THREE.Scene();
    scene.background = new THREE.Color('#7db1de');

    const camera = new THREE.PerspectiveCamera(65, mount.clientWidth / mount.clientHeight, 0.1, 5000);
    camera.position.set(-30, 20, 40);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(mount.clientWidth, mount.clientHeight);
    mount.appendChild(renderer.domElement);

    scene.add(new THREE.HemisphereLight(0xffffff, 0x445566, 1.0));
    const dir = new THREE.DirectionalLight(0xffffff, 0.8);
    dir.position.set(100, 120, 50);
    scene.add(dir);

    const ground = new THREE.Mesh(
      new THREE.PlaneGeometry(4000, 4000),
      new THREE.MeshStandardMaterial({ color: '#2f6a3a' })
    );
    ground.rotation.x = -Math.PI / 2;
    scene.add(ground);

    const plane = new THREE.Group();
    const fuselage = new THREE.Mesh(
      new THREE.BoxGeometry(5, 0.8, 1.2),
      new THREE.MeshStandardMaterial({ color: '#e6e6e6' })
    );
    const wing = new THREE.Mesh(
      new THREE.BoxGeometry(1.2, 0.15, 7),
      new THREE.MeshStandardMaterial({ color: '#aa2d2d' })
    );
    const tail = new THREE.Mesh(
      new THREE.BoxGeometry(1.0, 1.0, 0.2),
      new THREE.MeshStandardMaterial({ color: '#c7c7c7' })
    );
    tail.position.set(-2.2, 0.5, 0);
    plane.add(fuselage, wing, tail);
    plane.position.set(0, 12, 0);
    scene.add(plane);
    planeRef.current = plane;

    const onResize = () => {
      camera.aspect = mount.clientWidth / mount.clientHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(mount.clientWidth, mount.clientHeight);
    };
    window.addEventListener('resize', onResize);

    let raf = 0;
    const animate = () => {
      raf = requestAnimationFrame(animate);
      if (planeRef.current) {
        const p = planeRef.current;
        camera.position.lerp(new THREE.Vector3(p.position.x - 40, p.position.y + 18, p.position.z + 28), 0.08);
        camera.lookAt(p.position.x, p.position.y, p.position.z);
      }
      renderer.render(scene, camera);
    };
    animate();

    return () => {
      cancelAnimationFrame(raf);
      window.removeEventListener('resize', onResize);
      mount.removeChild(renderer.domElement);
      renderer.dispose();
    };
  }, []);

  useEffect(() => {
    let cancelled = false;
    loadSimBridge()
      .then((sim) => {
        if (cancelled) return;
        simRef.current = sim;
        setReady(true);
      })
      .catch((err) => {
        if (cancelled) return;
        setError(err.message);
      });

    return () => {
      cancelled = true;
    };
  }, []);

  useEffect(() => {
    const onKeyDown = (ev) => {
      keysRef.current.add(ev.code);
      if (ev.code === 'KeyR' && simRef.current) {
        simRef.current.reset();
      }
    };
    const onKeyUp = (ev) => keysRef.current.delete(ev.code);
    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
    };
  }, []);

  useEffect(() => {
    if (!ready) return;

    const control = { throttle: 0.6, pitch: 0.5, roll: 0.5, trigger: 0 };
    const interval = setInterval(() => {
      const keys = keysRef.current;
      if (keys.has('KeyW')) control.throttle = Math.min(1, control.throttle + 0.02);
      if (keys.has('KeyS')) control.throttle = Math.max(0, control.throttle - 0.02);

      let pitchAxis = 0;
      let rollAxis = 0;
      if (keys.has('ArrowUp')) pitchAxis += 1;
      if (keys.has('ArrowDown')) pitchAxis -= 1;
      if (keys.has('ArrowLeft')) rollAxis -= 1;
      if (keys.has('ArrowRight')) rollAxis += 1;
      control.pitch = 0.5 + 0.5 * pitchAxis;
      control.roll = 0.5 + 0.5 * rollAxis;
      control.trigger = keys.has('Space') ? 1 : 0;

      simRef.current.step(control);
      const next = simRef.current.telemetry();
      setTelemetry(next);

      if (planeRef.current) {
        planeRef.current.position.x += next.speed * 0.01;
        planeRef.current.position.y = Math.max(3, next.altitude * 0.01);
        planeRef.current.rotation.z = next.rollAttitude;
        planeRef.current.rotation.x = next.pitchAttitude;
      }
    }, DT_MS);

    return () => clearInterval(interval);
  }, [ready]);

  return (
    <div className="app">
      <div className="hud">
        <h1>C Flight Sim + React + Three.js</h1>
        <p className="hint">Controls: W/S throttle, Arrow keys pitch/roll, Space fire, R reset</p>
        {error ? <p className="error">{error}</p> : null}
        <pre>{`speed: ${fmt(telemetry.speed)} m/s
altitude: ${fmt(telemetry.altitude)} m
climb: ${fmt(telemetry.climbRate)} m/s
throttle: ${fmt(telemetry.throttle)}
pitch: ${fmt(telemetry.pitchAttitude)} rad
roll: ${fmt(telemetry.rollAttitude)} rad
turnRate: ${fmt(telemetry.turnRate)} rad/s
aoa: ${fmt(telemetry.aoaEstimate)} rad
health: ${fmt(telemetry.health, 1)}
ammo: ${telemetry.ammo}`}</pre>
        <p className="status">{ready ? 'WASM ready' : 'Loading WASM...'}</p>
      </div>
      <div ref={mountRef} className="viewport" />
    </div>
  );
}

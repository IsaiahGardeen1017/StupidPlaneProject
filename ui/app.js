import React, { useEffect, useMemo, useRef, useState } from 'https://esm.sh/react@18.3.1';
import { createRoot } from 'https://esm.sh/react-dom@18.3.1/client';
import { loadSimBridge } from './sim_bridge.js';

const dtMs = 1000 / 60;

function format(v, digits = 2) {
  return Number.isFinite(v) ? v.toFixed(digits) : '0.00';
}

function App() {
  const simRef = useRef(null);
  const keysRef = useRef(new Set());
  const [error, setError] = useState('');
  const [ready, setReady] = useState(false);
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
    let cancelled = false;

    loadSimBridge()
      .then((sim) => {
        if (cancelled) return;
        simRef.current = sim;
        setReady(true);
      })
      .catch((err) => {
        if (cancelled) return;
        setError(`Could not load WASM C simulation: ${err.message}`);
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
    const onKeyUp = (ev) => {
      keysRef.current.delete(ev.code);
    };
    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
    };
  }, []);

  const controlState = useRef({ throttle: 0.6, pitch: 0.5, roll: 0.5, trigger: 0 });

  useEffect(() => {
    if (!ready) return;

    const id = setInterval(() => {
      const sim = simRef.current;
      if (!sim) return;

      const keys = keysRef.current;
      const control = controlState.current;

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

      sim.step(control);
      setTelemetry(sim.telemetry());
    }, dtMs);

    return () => clearInterval(id);
  }, [ready]);

  const hint = useMemo(
    () => 'Controls: W/S throttle, Arrow keys pitch/roll, Space fire, R reset',
    []
  );

  return (
    React.createElement('main', { className: 'layout' },
      React.createElement('h1', null, 'StupidPlaneProject — C Flight Sim (WASM)'),
      error ? React.createElement('p', { className: 'error' }, error) : null,
      React.createElement('p', { className: 'hint' }, hint),
      React.createElement('section', { className: 'panel' },
        React.createElement('h2', null, 'Telemetry'),
        React.createElement('pre', null,
`speed: ${format(telemetry.speed)} m/s
altitude: ${format(telemetry.altitude)} m
climb: ${format(telemetry.climbRate)} m/s
throttle: ${format(telemetry.throttle)}
pitch: ${format(telemetry.pitchAttitude)} rad
roll: ${format(telemetry.rollAttitude)} rad
turnRate: ${format(telemetry.turnRate)} rad/s
aoa: ${format(telemetry.aoaEstimate)} rad
health: ${format(telemetry.health, 1)}
ammo: ${telemetry.ammo}`)
      ),
      React.createElement('p', { className: 'status' }, ready ? 'WASM ready' : 'Loading WASM...')
    ));
}

createRoot(document.getElementById('root')).render(React.createElement(App));

export async function loadSimBridge() {
  const wasmModulePath = `${import.meta.env.BASE_URL}wasm/sim.js`;
  const { default: createSimModule } = await import(/* @vite-ignore */ wasmModulePath);
  const mod = await createSimModule();

  const api = {
    init: mod.cwrap('sim_wasm_init', 'number', []),
    reset: mod.cwrap('sim_wasm_reset', null, []),
    setControl: mod.cwrap('sim_wasm_set_control', null, ['number', 'number', 'number', 'number']),
    tick: mod.cwrap('sim_wasm_tick', null, []),
    getSpeed: mod.cwrap('sim_wasm_get_speed', 'number', []),
    getAltitude: mod.cwrap('sim_wasm_get_altitude', 'number', []),
    getClimbRate: mod.cwrap('sim_wasm_get_climb_rate', 'number', []),
    getPitch: mod.cwrap('sim_wasm_get_pitch_attitude', 'number', []),
    getRoll: mod.cwrap('sim_wasm_get_roll_attitude', 'number', []),
    getTurnRate: mod.cwrap('sim_wasm_get_turn_rate', 'number', []),
    getAoa: mod.cwrap('sim_wasm_get_aoa_estimate', 'number', []),
    getThrottle: mod.cwrap('sim_wasm_get_throttle', 'number', []),
    getHealth: mod.cwrap('sim_wasm_get_health', 'number', []),
    getAmmo: mod.cwrap('sim_wasm_get_ammo', 'number', []),
  };

  if (api.init() !== 0) {
    throw new Error('Failed to initialize C simulation world.');
  }

  return {
    reset: () => api.reset(),
    step: (control) => {
      api.setControl(control.throttle, control.pitch, control.roll, control.trigger);
      api.tick();
    },
    telemetry: () => ({
      speed: api.getSpeed(),
      altitude: api.getAltitude(),
      climbRate: api.getClimbRate(),
      pitchAttitude: api.getPitch(),
      rollAttitude: api.getRoll(),
      turnRate: api.getTurnRate(),
      aoaEstimate: api.getAoa(),
      throttle: api.getThrottle(),
      health: api.getHealth(),
      ammo: api.getAmmo(),
    }),
  };
}

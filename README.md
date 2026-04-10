# StupidPlaneProject

Initial implementation of Phase 1 simulation core in C.

## Implemented so far

- Deterministic fixed-timestep world update (`sim_tick`) at configurable `dt`.
- Per-aircraft state: position, velocity, orientation, angular rates, throttle state, health, ammo.
- Data-driven curve sampling with linear interpolation for thrust, drag, lift, roll rate, and pitch rate.
- Basic combat model with ballistic projectiles, hit radius checks, health-based destruction, and optional friendly fire.
- Deterministic hashing utility for regression tests.
- World-config schema versioning and validation entrypoints (`sim_default_world_config`, `sim_validate_world_config`).
- Per-plane stat schema/versioning and validation entrypoints (`sim_default_plane_stats`, `sim_validate_plane_stats`).
- Aircraft telemetry extraction API for UI/HUD integration (`sim_get_aircraft_telemetry`).
- World snapshot export helper for HUD/update loops (`sim_get_world_snapshot`).
- World-config serialization/deserialization helpers for versioned preset round-trips.
- Lifecycle helpers (`sim_world_create`, `sim_world_reset`, `sim_world_destroy`) to simplify embedding/bindings.
- World metrics helper (`sim_get_world_metrics`) for deterministic scenario summaries.

## Build and test

### Prerequisites

- A C compiler that supports C11 (`cc`, `clang`, or `gcc`)
- `make`
- Node.js 22+ and npm (for the Vite React UI)
- Emscripten (`emcc`) to build the browser WASM module that powers the React UI

Install Emscripten (recommended via emsdk):

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

On macOS, install build tools with:

```bash
xcode-select --install
```

On Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential make
```

### Run simulation tests

```bash
make test
```

This runs `engine/tests/test_sim.c`, which currently validates:

1. Determinism (`same seed + same inputs => same world hash`)
2. Projectile damage interaction
3. Config schema validation and safe default fallback behavior
4. Telemetry extraction behavior for a spawned aircraft
5. World snapshot export behavior
6. Config serialization/deserialization round-trip behavior
7. Heap lifecycle helper behavior (`create/reset/destroy`)
8. World metrics export behavior

`make` is used for **building/running the C test binary** and for building the browser WASM module (`make wasm`).

## Browser flight UI (React + C/WASM)

A lightweight browser UI is available at `ui/index.html` and is backed by the C simulation compiled to WASM.

### Features
- Fly a plane with keyboard controls (`W/S`, arrow keys, `Space`, `R`)
- Live telemetry panel (speed, altitude, climb, throttle, attitude, turn rate, AoA estimate)
- Telemetry panel sourced from C sim state via WASM bridge
- Keyboard control mapped to C sim controller inputs
- Three.js-rendered world + player aircraft in the React page

### Run locally (React UI backed by C/WASM)

1) Build the WASM module:

```bash
make wasm
```

2) Install UI dependencies and run Vite:

```bash
cd ui
npm install
npm run dev
```

3) Open the Vite URL printed in terminal (usually `http://localhost:5173`).

### Quick local workflow

From repo root:

```bash
make test
make wasm
cd ui
npm install
npm run dev
```

For production/static output:

```bash
cd ui
npm run build
```

## GitHub Actions + GitHub Pages deployment

This repo now includes `.github/workflows/pages.yml` that:

1. Runs `make test` on every push to `main`
2. If tests pass, deploys the `ui/` folder to GitHub Pages

### One-time GitHub repo settings

1. Go to **Settings → Pages**
2. Under **Build and deployment**, set **Source** to **GitHub Actions**
3. Push to `main` (or run workflow manually from **Actions** tab)

After deploy, the site URL appears in the workflow run and under Pages settings.

## Next likely steps

- Expand physics fidelity (explicit AoA estimate from velocity/body frame, yaw and slip effects).
- Add scenario/config schema serialization and versioning.
- Expose C API boundary suitable for Emscripten/WASM and TypeScript bindings.
- Expand telemetry fields and add snapshot/export helpers for Phase 2 HUD and tuning UI.

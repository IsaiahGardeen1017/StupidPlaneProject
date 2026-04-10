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
- Python 3 (only if you want to run the local UI server)
- Node.js is **not required yet** for the current checked-in build/test flow

On macOS, install build tools with:

```bash
xcode-select --install
```

On Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential make python3
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

## Three.js flight tuning UI

A lightweight browser UI is now available at `ui/index.html` for single-plane open-world tuning.

### Features
- Fly a plane with keyboard controls (`W/S`, arrow keys, `Space`, `R`)
- Live telemetry panel (speed, altitude, climb, throttle, attitude, turn rate, AoA estimate)
- Runtime tuning sliders for thrust, drag, lift, pitch rate, and roll rate (with live numeric values)
- Preset save/load via `localStorage` and import/export via JSON text

### Run locally
Use any static file server from repo root (example):

```bash
python -m http.server 8080
```

Then open `http://localhost:8080/ui/`.

### Quick local workflow

From repo root:

```bash
make test
python -m http.server 8080
```

Then in browser:

- `http://localhost:8080/ui/`

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

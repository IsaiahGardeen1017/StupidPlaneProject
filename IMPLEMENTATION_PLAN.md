# TypeScript Plane Stats → C Simulation Integration Plan

## Goals

1. Author and iterate aircraft stats in TypeScript.
2. Generate deterministic, C-readable plane data during build.
3. Refactor engine config naming from `sim_config` to `plane_stats` semantics where appropriate.
4. Ensure current physics consumes those plane stats rather than hardcoded/implicit assumptions.

---

## Proposed architecture

### 1) Split world-level config from per-plane stats

Current `sim_config` mixes world settings (dt, gravity, projectile params, friendly fire) with aerodynamic curves that are really aircraft characteristics.

Refactor to:

- `sim_world_config` (global/session settings)
  - dt
  - gravity
  - world floor altitude
  - projectile properties
  - friendly fire
  - trigger cooldown
- `sim_plane_stats` (per aircraft type)
  - mass/weight
  - wing area
  - lift coefficient vs AoA curve
  - parasite drag params
  - induced drag factor (`AR * e`)
  - thrust curve(s)
  - control authority curves/rates

Then each `sim_aircraft` instance should reference a plane type/stats record (e.g., `plane_type_id` index into a table in world state).

### 2) Build-time TypeScript compiler pipeline

Add a script under `engine/typescriptBuild/` that:

- Imports `planes.ts`
- Validates all records against runtime constraints (sorted curves, positive values, no NaN, mandatory fields)
- Emits two generated files:
  - `engine/generated/plane_stats.generated.h`
  - `engine/generated/plane_stats.generated.c`

The generated C files contain:

- `static const sim_curve_point` arrays for each curve
- `const sim_plane_stats g_plane_stats[]`
- `const size_t g_plane_stats_count`
- lookup helper by string id if needed (or compile-time enum mapping)

This keeps C runtime simple/deterministic and lets TypeScript own authoring.

### 3) Schema/versioning strategy

Introduce:

- `SIM_PLANE_STATS_SCHEMA_VERSION`
- `SIM_WORLD_CONFIG_SCHEMA_VERSION`

Both are serialized and validated separately. This avoids coupling world settings evolution with aircraft evolution.

---

## Suggested implementation phases

### Phase A — groundwork/refactor

1. Add `sim_plane_stats` type and `sim_world_config` type.
2. Rename current `sim_config` APIs in C to world-config-specific names.
3. Update world init/reset/create to accept both world config and plane stats table.
4. Make physics update read force/control parameters from selected `sim_plane_stats`.

### Phase B — TypeScript generation

1. Add `tsconfig` + node script (`generatePlaneStats.ts`).
2. Generate C header/source from `planes.ts`.
3. Wire Makefile target (`make gen-plane-stats`) and include generated C in test build.
4. Add CI check that generated files are up-to-date.

## Review of current `PlaneStats` fields

Current fields are a good start, but a few additions will likely be needed for stable behavior and tuning:

Recommended additions:

- `emptyMassKg` or `massKg` (rename from generic `weight`; weight is force in physics terms)
- `maxAmmo` / weapon config (if per-plane armament differs)
- `maxHealth` (if survivability should vary by plane)
- `maxThrottleResponseRate` (engine spool dynamics)
- `rollRateByInput` and `pitchRateByInput` per plane (currently global in C config)
- optional `thrustBySpeed` or 2D thrust map (altitude + speed) for better fidelity
- stall behavior controls:
  - `clMax`
  - `stallAoaDeg`
  - `postStallLiftScale`

Validation rules to enforce:

- curves strictly monotonic in `x`
- no duplicate `x`
- all coefficients finite and non-negative where expected
- AoA curve domain clearly defined (degrees vs radians; pick one and enforce)

---

## BF 109 sanity check (quick review)

Given:

- `weight: 2700`
- `wingSurfaceArea: 16.2`
- `parasiteDragCoefficient: 0.03`
- `parasiteDragReferenceArea: 0.3`
- `thrustByAirDensity` points near 7000–8000 N
- lift curve peaks around `Cl ~1.45` at 15 deg

Assessment:

- `weight: 2700` is plausible **if interpreted as mass in kg** (Bf 109 variants are in that rough class).
- `wingSurfaceArea: 16.2` looks plausible.
- `Cd0 = 0.03` is reasonable as a game-tuning baseline.
- `Cl` peak around 1.4–1.5 is plausible for pre/post-stall simplified table.
- `thrust 7000–8000 N` can be plausible for a piston-prop equivalent thrust abstraction, but you should define whether this is:
  - equivalent propulsive force at a reference speed, or
  - static thrust approximation.

Density axis clarification:

- Use `thrustByAirDensity` explicitly for the thrust curve x-axis.
- In engine physics, compute air density from aircraft altitude using a fixed-temperature atmosphere approximation, then sample the curve by density.

---

## Immediate next coding step

Start with **Phase A** only:

1. Introduce `sim_plane_stats` alongside existing `sim_config` (non-breaking).
2. Move aerodynamic/control curves into `sim_plane_stats`.
3. Assign one plane stats record to each spawned aircraft.
4. Keep old defaults by creating a built-in `default_plane_stats` record.

This gives a safe migration path before code generation work.

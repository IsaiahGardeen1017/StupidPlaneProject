# StupidPlaneProject — Feasibility & Build Plan

## 1) Does the idea make sense?

Yes. The concept is coherent and very feasible if you build it in phases.

You are combining:
- **A physics-driven flight/combat simulator** (deterministic simulation loop)
- **Neuroevolution training** (genetic algorithm + neural network weights)
- **A lightweight game shell** (UI, controls, replay, save/load models)

The key to success is to avoid trying to build “War Thunder lite” all at once. Start with simplified but believable physics and iterate.

---

## 2) Recommended technical approach

## Core architectural split

Use a 3-layer architecture:
1. **Simulation Core** (deterministic, headless, fast)
2. **Training Engine** (selection/evolution orchestration)
3. **Visualization/UI** (web front end + optional player control)

This lets you train at high speed without rendering, then replay best runs visually.

---

## 3) Stack recommendation (best fit for your skills + web target, no-GC preference)

### Recommended Stack A (strong default for your preferences)

- **Frontend/UI:** TypeScript + React + WebGL/Three.js (or Pixi for simpler visuals)
- **Simulation Core:** **Zig** (or C) compiled to **WebAssembly** (WASM)
- **Training:** Runs in WASM + Web Workers for parallelism
- **Backend (optional):** ASP.NET Core for leaderboard/model storage/job orchestration

Why this stack:
- You keep web deployment easy.
- Zig/C are non-GC systems languages and match your preference to avoid Rust.
- WASM still gives deterministic, high-performance browser execution.
- TypeScript remains your control layer and UI strength.
- C# backend is a great fit if/when you need cloud training or persistent services.

### Alternative Stack B (if you want all-C#)

- **Frontend:** Blazor WebAssembly or TS/React frontend
- **Simulation/Training:** C# with .NET 8, run server-side for scale
- **Client:** Streams state/replays

Tradeoff: simpler language story, but browser-side heavy training is weaker than WASM systems-language cores for raw perf.

### Tooling callout (Zig vs C in WASM)

- **C + Emscripten:** most mature ecosystem, many examples, good default if you want battle-tested tooling.
- **Zig + WASM target:** cleaner build ergonomics and modern language design, but fewer large examples than C/Emscripten.
- If you want fastest path to production confidence today, pick **C + Emscripten**.

### Avoid for v1

- Complex multi-runtime architectures early (e.g., half the sim in JS, half in server, half in client). Keep one deterministic core first.

---

## 4) Physics fidelity strategy

Model the aircraft with “good-enough realism” first:

State per aircraft:
- Position (x, y, z)
- Velocity vector
- Orientation (quaternion)
- Angular velocity
- Engine/throttle state
- Damage/health/ammo

Forces/torques per tick:
- Thrust = f(altitude, speed, throttle)
- Drag = f(altitude, speed, AoA, flap/gear/control-surface penalties)
- Lift = f(altitude, speed, AoA)
- Gravity
- Control torques (pitch/roll/yaw authority curves)

Do this as lookup curves/tables with interpolation (not huge CFD-style equations). It is easier to tune and keeps perf high.

---

## 5) AI/training design (neuroevolution)

Use **fixed-topology MLP + genetic mutations** for v1.

Inputs (example):
- Own speed, altitude, heading, pitch/roll rates
- Target relative bearing/elevation/range/range-rate
- Energy state proxy
- Ammo + health
- Nearby threat indicators (top-N)

Outputs (controller interface per tick):
- `throttle` in `[0, 1]`
- `pitch` in `[0, 1]` (map to control surface range internally)
- `roll` in `[0, 1]` (map to control surface range internally)
- `trigger` in `{0, 1}`
- Optional `yaw` in `[0, 1]` (start without it, add only if needed)

Yaw recommendation:
- Start with **no direct yaw control** in v1 (rudder auto-coordinated via a stability helper).
- Add explicit yaw later if you observe bad behaviors (e.g., poor gun tracking at low speed, inability to correct sideslip, weak scissors/defensive maneuvers).
- This keeps the action space smaller for initial neuroevolution and usually trains faster.

Fitness function (multi-objective weighted score):
- Survival time
- Kills/damage dealt
- Damage avoided
- Energy efficiency/flight stability penalties

Evolution loop:
1. Simulate N agents in batches
2. Rank by fitness
3. Keep elites
4. Crossover + mutation on weights
5. Repeat

Start with **co-evolution in small arenas** before full “many planes in huge world.”

---

## 6) Determinism and reproducibility (critical)

To support save/load and comparable training:
- Fixed timestep (e.g., 30 or 60 Hz)
- Seeded RNG everywhere
- Deterministic math path where possible
- Save artifacts: genome/weights, config, seed, engine version

Model package format suggestion (JSON + binary weights):
- `metadata.json` (version, topology, fitness stats)
- `weights.bin`
- `scenario_config.json`

Replay/event stream package (for future server/multiplayer portability):
- `events.bin` (append-only per-tick command/state delta stream)
- `snapshots/` (periodic keyframes for fast seeking)
- `manifest.json` (tick rate, schema version, compression format)

---

## 7) Performance + deployment plan

You can scale to large plane counts if you:
- Run **headless training** (no rendering)
- Use **spatial partitioning** (uniform grid) for targeting/collision checks
- Parallelize simulations via workers
- Keep physics simple and branch-light

Rule of thumb:
- Browser-only can handle meaningful counts for prototyping.
- Very large populations or long generations likely move to server jobs later.

Design for “local now, server later” from day one:
- Treat simulation as a pure function of `(seed, initial state, input stream)`.
- Make the UI consume a **state/event stream API**, even when producer is local WASM.
- Later, a remote server can publish the same stream format with minimal UI changes.
- The same stream can be persisted as a replay file and scrubbed in a viewer.

---

## 8) Milestone roadmap

### Milestone 0 — Technical spike (1–2 weeks)
- Fixed-step sim loop
- One aircraft model flying stably
- Replay viewer from saved event stream
- Deterministic seed replay validation

### Milestone 1 — Combat sandbox (2–4 weeks)
- 2–4 planes dogfighting
- Gunnery/hit model
- Basic fitness and evolution
- Save/load best genome

### Milestone 2 — Scaled evolution (3–6 weeks)
- Population batches
- Parallel workers
- UI controls for world size, plane count, time limit
- Charts: fitness by generation
- Stable streaming schema (works locally and over websocket/server-sent stream)

### Milestone 3 — “Playable experiment” (2–4 weeks)
- Player can spawn and fight trained agents
- Preset aircraft archetypes
- Export/import AI packages
- Remote sim mode toggle (local producer vs server stream producer)

### Milestone 4 — Depth and polish
- Better aircraft parameter editor
- Team tactics behaviors
- Optional backend training farm

---

## 9) Suggested repo structure

```text
/apps
  /web-ui               # React/TS front end
/packages
  /sim-core             # Zig/C -> WASM deterministic simulator
  /trainer              # Evolution orchestration logic
  /shared-schema        # Scenario/model save formats
  /stream-schema        # Event/state stream + replay format
/services
  /api                  # Optional ASP.NET Core service
/docs
  architecture.md
  physics-model.md
  ai-training.md
```

---

## 10) Risks and mitigations

1. **Physics too complex too early**
   - Mitigation: table-driven curves + iterate.
2. **Training collapse/local optima**
   - Mitigation: novelty bonus, curriculum scenarios, periodic random immigrants.
3. **Performance bottlenecks**
   - Mitigation: profile early, headless mode first, spatial partitioning.
4. **Non-deterministic runs**
   - Mitigation: fixed timestep + seed control + replay tests in CI.

---

## 11) Bottom line feasibility

This project is absolutely feasible.

Best path:
- Build a deterministic sim core first.
- Layer neuroevolution second.
- Add web visualization and player dogfight modes third.

If executed incrementally, you can have a compelling prototype quickly and grow realism over time.

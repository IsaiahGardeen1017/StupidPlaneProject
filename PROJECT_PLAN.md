# StupidPlaneProject — Project Specification

## 1) Purpose

Build a browser-playable WWII-style air combat simulation where realistic-feeling aircraft behavior is the primary objective. AI neuroevolution is a later layer built on top of a validated flight/combat engine.

This specification is execution-focused and prioritized by implementation order.

---

## 2) Priority Order

1. **Air combat simulation engine**
2. **Single-plane player flight UI in open world** (for flight tuning/feel validation)
3. **Neural training for AI flight/combat controllers**
4. **Optional later features**: multiplayer, progression, missions, replay systems

Work must proceed in this order unless explicitly re-prioritized.

---

## 3) Technology Stack

- **Simulation core:** C
- **WASM toolchain:** Emscripten
- **Web client:** TypeScript + React
- **Rendering:** Three.js

---

## 4) Phase 1 — Air Combat Simulation Engine (Top Priority)

## 4.1 Objectives
- Build deterministic fixed-timestep flight/combat simulation.
- Produce aircraft motion that feels believable and tunable.
- Support multiple aircraft in one world with combat interactions.

## 4.2 Core simulation requirements
- Fixed tick rate (default 60 Hz).
- Deterministic update ordering.
- Seeded RNG where randomness is used.
- Per-aircraft state includes position, velocity, orientation, angular rates, throttle state, health, ammo.

## 4.3 Flight model requirements
Use data-driven curve/table lookup with interpolation:
- Thrust = f(altitude, speed, throttle)
- Drag = f(altitude, speed, AoA, control-surface penalties)
- Lift = f(altitude, speed, AoA)
- RollRate = f(altitude, speed, rollInput)
- PitchRate = f(altitude, speed, pitchInput)

## 4.4 Combat model requirements (v1)
- Ballistic projectiles with travel time.
- Hit detection against aircraft collision volumes.
- Damage model: health pool + destroy state.
- Friendly fire behavior configurable.

## 4.5 Controller interface (engine contract)
Control outputs per tick:
- `throttle` in `[0,1]`
- `pitch` in `[0,1]`
- `roll` in `[0,1]`
- `trigger` in `{0,1}`
- No direct yaw output in v1

Engine controllers will be either players or neaural ai AI agents.

---

## 5) Phase 2 — Single-Plane Flight UI (Critical Tuning Tool)

## 5.1 Objectives
Provide a direct pilotable open-world mode to tune flight characteristics and identify acceptable aircraft-stat ranges.

## 5.2 Required UI capabilities
- Spawn one plane in open world.
- Real-time control inputs mapped to engine controller interface.
- HUD telemetry for tuning:
  - speed, altitude, climb rate
  - throttle value
  - pitch/roll attitude
  - turn rate
  - AoA estimate
- Runtime parameter panel for live tuning of selected aircraft curves/coefficients.
- Reset/restart scenario controls.

## 5.3 Tuning workflow requirements
- User can adjust flight parameters without recompiling engine.
- User can save a tuning preset and reload it.
- UI must expose enough telemetry to diagnose instability and “feel.”

## 5.4 Acceptance criteria
- Human pilot can take off/maintain controlled flight/perform turns consistently.
- Parameter changes visibly affect behavior in expected directions.
- Tuning presets can be exported/imported.

---

## 6) Phase 3 — Neural Training for AI Controllers

## 6.1 Objectives
Train AI controllers that can fly and fight using the same control interface as the player mapping.

## 6.2 Training approach (v1)
- Fixed-topology MLP controller.
- Evolutionary optimization loop (selection + mutation/crossover).
- Batch simulation in headless mode for throughput.

## 6.3 Training loop
1. Initialize population of policies.
2. Simulate combat episodes.
3. Score fitness.
4. Select elites.
5. Generate offspring.
6. Repeat generations.

## 6.4 Fitness system requirements
- Fitness composition must be configurable (weights and terms).
- Initial available terms:
  - survival time
  - damage dealt / kills
  - damage taken penalty
  - crash penalty
- Final heuristic will be user-authored during training iteration.

## 6.5 Model persistence
- Save/load trained model artifacts (`metadata` + weights + scenario config).
- Record training run metadata (seed, generation count, best score).

## 6.6 Acceptance criteria
- Measurable improvement over generations in at least one benchmark scenario.
- Saved model can be reloaded and used in a live simulation session.

---

## 7) Phase 4 — Optional/Later Features

These features are explicitly lower priority and may never be implemented:
- Multiplayer
- Progression systems
- Mission framework
- Replay productization layer

No Phase 4 work begins until Phases 1–3 are functional and accepted.

---

## 8) Determinism and Test Requirements

## 8.1 Determinism requirements
- Fixed timestep only.
- Seed-controlled RNG.
- Stable entity iteration order.
- Versioned simulation config schema.

## 8.2 Minimum validation suite
- Determinism regression test (same seed/config => same output metrics).
- Flight stability scenario test.
- Combat episode completion test.
- Training loop smoke test (short run).

---

## 9) Milestones and Gates

## M1 — Engine Core
Deliver:
- deterministic tick loop
- aircraft state integration
- force/torque model with curves
- ballistic projectile combat

Gate:
- passes Phase 1 acceptance criteria.

## M2 — Pilotable Open-World UI
Deliver:
- single-plane player mode
- HUD telemetry
- live parameter tuning + preset save/load

Gate:
- passes Phase 2 acceptance criteria.

## M3 — AI Neuroevolution
Deliver:
- population training loop
- configurable fitness terms
- save/load trained policies

Gate:
- passes Phase 3 acceptance criteria.

## M4 — Optional Feature Track
Deliver:
- only if approved after M1–M3 outcomes.

---

